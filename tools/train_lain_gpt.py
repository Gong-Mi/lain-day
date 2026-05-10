#!/usr/bin/env python3
"""
train_lain_gpt.py - 用 tinygrad 训练微型 GPT，用于 lain-day 游戏文本生成

用法:
  # 快速训练（默认 ~2M 参数，约 30-60 分钟）
  python3 tools/train_lain_gpt.py

  # 更大模型（~8M 参数，质量更好但更慢）
  python3 tools/train_lain_gpt.py --dim 256 --n_layers 6 --n_heads 8 --steps 5000

  # 继续训练
  python3 tools/train_lain_gpt.py --resume

  # 只生成文本
  python3 tools/train_lain_gpt.py --gen_only --prompt "你走进房间，"
"""
import os
import sys
import math
import time
import argparse
import pickle
from pathlib import Path
import numpy as np

# tinygrad
from tinygrad import Tensor
from tinygrad.nn import Embedding, Linear, LayerNorm
from tinygrad.nn.optim import AdamW
from tinygrad.nn.state import get_state_dict, safe_save, safe_load

# 注意：如果首次运行遇到 msync 错误，需要修改 tinygrad 的 ops_cpu.py
# 已为你自动修复：/data/data/com.termux/files/usr/lib/python3.13/site-packages/tinygrad/runtime/ops_cpu.py


class CausalSelfAttention:
    def __init__(self, dim, n_heads):
        assert dim % n_heads == 0
        self.n_heads = n_heads
        self.head_dim = dim // n_heads
        self.c_attn = Linear(dim, 3 * dim)
        self.c_proj = Linear(dim, dim)

    def __call__(self, x):
        B, T, C = x.shape
        qkv = self.c_attn(x).reshape(B, T, 3, self.n_heads, self.head_dim).permute(2, 0, 3, 1, 4)
        q, k, v = qkv[0], qkv[1], qkv[2]
        attn = (q @ k.transpose(-2, -1)) * (1.0 / math.sqrt(self.head_dim))
        mask = Tensor(np.tril(np.ones((T, T), dtype=np.float32)).reshape(1, 1, T, T))
        attn = attn.masked_fill(mask == 0, float('-inf'))
        attn = attn.softmax(-1)
        y = (attn @ v).transpose(1, 2).reshape(B, T, C)
        return self.c_proj(y)


class MLP:
    def __init__(self, dim, hidden_dim):
        self.c_fc = Linear(dim, hidden_dim)
        self.c_proj = Linear(hidden_dim, dim)

    def __call__(self, x):
        return self.c_proj(self.c_fc(x).gelu())


class Block:
    def __init__(self, dim, n_heads, mlp_ratio=4):
        self.ln_1 = LayerNorm(dim)
        self.attn = CausalSelfAttention(dim, n_heads)
        self.ln_2 = LayerNorm(dim)
        self.mlp = MLP(dim, dim * mlp_ratio)

    def __call__(self, x):
        x = x + self.attn(self.ln_1(x))
        x = x + self.mlp(self.ln_2(x))
        return x


class LainGPT:
    def __init__(self, vocab_size, dim, n_layers, n_heads, context_length):
        self.context_length = context_length
        self.wte = Embedding(vocab_size, dim)
        self.wpe = Embedding(context_length, dim)
        self.blocks = [Block(dim, n_heads) for _ in range(n_layers)]
        self.ln_f = LayerNorm(dim)
        self.lm_head = Linear(dim, vocab_size, bias=False)

    def __call__(self, idx):
        B, T = idx.shape
        assert T <= self.context_length
        tok_emb = self.wte(idx)
        pos_emb = self.wpe(Tensor.arange(T))
        x = tok_emb + pos_emb
        for block in self.blocks:
            x = block(x)
        x = self.ln_f(x)
        logits = self.lm_head(x)
        return logits


class CharTokenizer:
    def __init__(self):
        self.chars = []
        self.stoi = {}
        self.itos = {}

    def train(self, text):
        self.chars = sorted(list(set(text)))
        self.stoi = {ch: i for i, ch in enumerate(self.chars)}
        self.itos = {i: ch for i, ch in enumerate(self.chars)}

    def encode(self, text):
        return [self.stoi.get(c, 0) for c in text]

    def decode(self, ids):
        return ''.join([self.itos.get(i, '?') for i in ids])

    @property
    def vocab_size(self):
        return len(self.chars)

    def save(self, path):
        with open(path, 'wb') as f:
            pickle.dump({'chars': self.chars}, f)

    def load(self, path):
        with open(path, 'rb') as f:
            data = pickle.load(f)
        self.chars = data['chars']
        self.stoi = {ch: i for i, ch in enumerate(self.chars)}
        self.itos = {i: ch for i, ch in enumerate(self.chars)}


def get_batch(data, batch_size, context_length):
    ix = np.random.randint(0, len(data) - context_length, size=(batch_size,))
    x = np.stack([data[i:i+context_length] for i in ix])
    y = np.stack([data[i+1:i+context_length+1] for i in ix])
    return Tensor(x), Tensor(y)


def estimate_loss(model, data, batch_size, context_length, eval_iters=20):
    losses = []
    for _ in range(eval_iters):
        xb, yb = get_batch(data, batch_size, context_length)
        logits = model(xb)
        loss = logits.sparse_categorical_crossentropy(yb)
        losses.append(loss.numpy())
    return float(np.mean(losses))


def generate(model, tokenizer, prompt, max_new_tokens=100, temperature=1.0, top_k=None):
    idx = Tensor([tokenizer.encode(prompt)])
    for _ in range(max_new_tokens):
        idx_cond = idx if idx.shape[1] <= model.context_length else idx[:, -model.context_length:]
        logits = model(idx_cond)
        logits = logits[:, -1, :] / temperature
        if top_k is not None:
            v, _ = logits.topk(top_k, dim=-1)
            logits = logits.masked_fill(logits < v[:, [-1]], float('-inf'))
        probs = logits.softmax(-1).numpy().flatten()
        next_id = np.random.choice(len(probs), p=probs)
        idx_np = idx.numpy().flatten()
        idx = Tensor(np.concatenate([idx_np, [next_id]]).reshape(1, -1))
    return tokenizer.decode(idx.numpy().flatten().tolist())


def main():
    parser = argparse.ArgumentParser(description='Train a tiny GPT for lain-day text generation')
    parser.add_argument('--data', default='/data/data/com.termux/files/home/lain_day_train.txt', help='Training text file')
    parser.add_argument('--dim', type=int, default=192, help='Model dimension')
    parser.add_argument('--n_layers', type=int, default=4, help='Number of transformer layers')
    parser.add_argument('--n_heads', type=int, default=4, help='Number of attention heads')
    parser.add_argument('--context', type=int, default=128, help='Context length')
    parser.add_argument('--batch_size', type=int, default=8, help='Batch size')
    parser.add_argument('--lr', type=float, default=1e-3, help='Learning rate')
    parser.add_argument('--steps', type=int, default=2000, help='Training steps')
    parser.add_argument('--eval_interval', type=int, default=200, help='Evaluation interval')
    parser.add_argument('--save_interval', type=int, default=500, help='Save interval')
    parser.add_argument('--out_dir', default='/data/data/com.termux/files/home/lain_gpt', help='Output directory')
    parser.add_argument('--resume', action='store_true', help='Resume from checkpoint')
    parser.add_argument('--gen_only', action='store_true', help='Only generate, no training')
    parser.add_argument('--prompt', default='玲音站在走廊上，', help='Generation prompt')
    parser.add_argument('--gen_tokens', type=int, default=100, help='Tokens to generate')
    args = parser.parse_args()

    os.makedirs(args.out_dir, exist_ok=True)

    # 提取数据（如果不存在）
    if not os.path.exists(args.data):
        print(f"Data file not found: {args.data}")
        print("Extracting game text from lain-day...")
        extract_script = """
import json, os, glob
chars = set()
texts = []
base = '/data/data/com.termux/files/home/lain-day'
for f in glob.glob(base + '/data/strings_extra/*.json'):
    with open(f, 'r', encoding='utf-8') as fp:
        data = json.load(fp)
        for k, v in data.items():
            if isinstance(v, str):
                texts.append(v)
for f in glob.glob(base + '/data/scenes/**/*.ssl', recursive=True):
    with open(f, 'r', encoding='utf-8') as fp:
        texts.append(fp.read())
for f in glob.glob(base + '/**/*.txt', recursive=True):
    if 'target' not in f and 'build' not in f and 'node_modules' not in f:
        with open(f, 'r', encoding='utf-8') as fp:
            texts.append(fp.read())
out = '/data/data/com.termux/files/home/lain_day_train.txt'
with open(out, 'w', encoding='utf-8') as fp:
    fp.write('\\n\\n'.join(texts))
print(f'Extracted {len(texts)} texts, {sum(len(t) for t in texts)} chars to {out}')
"""
        os.system(f"python3 -c '{extract_script}'")

    with open(args.data, 'r', encoding='utf-8') as f:
        text = f.read()

    # Tokenizer
    tokenizer_path = os.path.join(args.out_dir, 'tokenizer.pkl')
    tokenizer = CharTokenizer()
    if os.path.exists(tokenizer_path):
        print(f"Loading tokenizer from {tokenizer_path}")
        tokenizer.load(tokenizer_path)
    else:
        print("Training tokenizer...")
        tokenizer.train(text)
        tokenizer.save(tokenizer_path)

    print(f"Vocab size: {tokenizer.vocab_size}")

    # Encode
    data = np.array(tokenizer.encode(text), dtype=np.int32)
    n = int(0.9 * len(data))
    train_data = data[:n]
    val_data = data[n:]
    print(f"Train tokens: {len(train_data)}, Val tokens: {len(val_data)}")

    # Model
    model = LainGPT(
        vocab_size=tokenizer.vocab_size,
        dim=args.dim,
        n_layers=args.n_layers,
        n_heads=args.n_heads,
        context_length=args.context
    )

    # Count params
    state_dict = get_state_dict(model)
    n_params = sum(p.numel() for p in state_dict.values())
    print(f"Model params: {n_params:,} ({n_params/1e6:.2f}M)")

    # Optimizer
    optimizer = AdamW(state_dict.values(), lr=args.lr)

    # Resume or generate-only
    model_path = os.path.join(args.out_dir, 'model.safetensors')
    step = 0
    if (args.resume or args.gen_only) and os.path.exists(model_path):
        print(f"Loading model from {model_path}")
        loaded = safe_load(model_path)
        for k, v in state_dict.items():
            if k in loaded:
                v.assign(loaded[k])

    if args.gen_only:
        if not os.path.exists(model_path):
            print("No model found for generation. Train first.")
            sys.exit(1)
        print(f"\nPrompt: {args.prompt}")
        print("Generating...")
        out = generate(model, tokenizer, args.prompt, max_new_tokens=args.gen_tokens, temperature=0.8)
        print(f"Output: {out}")
        return

    # Training
    Tensor.training = True
    print(f"\nStarting training for {args.steps} steps...")
    print(f"Config: dim={args.dim}, layers={args.n_layers}, heads={args.n_heads}, ctx={args.context}, batch={args.batch_size}")
    start_time = time.time()

    for step in range(step, args.steps):
        xb, yb = get_batch(train_data, args.batch_size, args.context)
        logits = model(xb)
        loss = logits.sparse_categorical_crossentropy(yb)

        optimizer.zero_grad()
        loss.backward()
        optimizer.step()

        if step % 50 == 0:
            elapsed = time.time() - start_time
            it_s = (step + 1) / elapsed if elapsed > 0 else 0
            print(f"step {step:5d}/{args.steps} | loss {loss.numpy():.4f} | {it_s:.2f} it/s | {elapsed:.0f}s")

        if step > 0 and step % args.eval_interval == 0:
            print("Evaluating...")
            train_loss = estimate_loss(model, train_data, args.batch_size, args.context, 10)
            val_loss = estimate_loss(model, val_data, args.batch_size, args.context, 10)
            print(f"=== step {step} | train {train_loss:.4f} | val {val_loss:.4f} ===")

        if step > 0 and step % args.save_interval == 0:
            safe_save(state_dict, model_path)
            print(f"Saved checkpoint to {model_path}")

    # Final save
    safe_save(state_dict, model_path)
    print(f"\nFinal model saved to {model_path}")

    # Generate sample
    print("\n--- Sample Generation ---")
    out = generate(model, tokenizer, args.prompt, max_new_tokens=args.gen_tokens, temperature=0.8)
    print(f"Prompt: {args.prompt}")
    print(f"Output: {out}")


if __name__ == '__main__':
    main()
