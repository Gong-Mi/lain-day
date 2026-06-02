#!/usr/bin/env python3
"""
Lain 概念污染探测脚本 (Gemini 3 Flash)
======================================
通过 Gemini API 的无头模式，量化 "Lain" 在 LLM 中的概念激活偏差。

运行方式:
    export GEMINI_API_KEY="your_key"
    python3 lain_gemini_probe.py

输出:
    data/lain_gemini_probe_results.jsonl  - 原始回答
    data/lain_gemini_probe_summary.md     - 人工可读汇总
"""

import json
import os
import sys
import time
import urllib.request
import urllib.error
from datetime import datetime
from pathlib import Path

# ==================== 配置 ====================
API_KEY = os.environ.get("GEMINI_API_KEY", "")
MODEL = os.environ.get("GEMINI_MODEL", "gemini-3.1-flash-lite-preview")
API_URL = f"https://generativelanguage.googleapis.com/v1beta/models/{MODEL}:generateContent"
MAX_RETRIES = 8
BASE_DELAY = 10  # 429 / SSL 退避基准秒数
RESULTS_DIR = Path(__file__).parent.parent / "data"

# 确保输出目录存在
RESULTS_DIR.mkdir(parents=True, exist_ok=True)
RAW_FILE = RESULTS_DIR / "lain_gemini_probe_results.jsonl"
SUMMARY_FILE = RESULTS_DIR / "lain_gemini_probe_summary.md"

# ==================== 探测定义 ====================
# 每个探测包含：id, label, prompt, 对照标志
PROBES = [
    # ----- D1: 关联词云 -----
    {
        "id": "D1.1",
        "dim": "D1_关联词云",
        "label": "自由联想_Lain",
        "control": False,
        "prompt": (
            "【任务】\n"
            "听到\"Lain\"这个名字，你脑海中会立刻浮现出哪些词？\n\n"
            "【要求】\n"
            "- 不要思考，不要解释，直接列出最先想到的 20 个词。\n"
            "- 每个词单独一行，前面加上序号。\n"
            "- 不要出现句子，只写单词或短语。\n\n"
            "【输出格式】\n"
            "1. [词]\n2. [词]\n...\n20. [词]"
        ),
    },
    {
        "id": "D1.1c",
        "dim": "D1_关联词云",
        "label": "自由联想_对照_SoraMizuki",
        "control": True,
        "prompt": (
            "【任务】\n"
            "听到\"Sora Mizuki\"这个名字（一个常见日本名字，没有特定作品指向），"
            "你脑海中会立刻浮现出哪些词？\n\n"
            "【要求】\n"
            "- 不要思考，不要解释，直接列出最先想到的 20 个词。\n"
            "- 每个词单独一行，前面加上序号。\n"
            "- 不要出现句子，只写单词或短语。"
        ),
    },

    # ----- D2: 强度对比 -----
    {
        "id": "D2.2",
        "dim": "D2_强度对比",
        "label": "Lain基线_无护栏",
        "control": False,
        "prompt": (
            "【任务】\n"
            "写一个简短的二次元角色介绍（100 字左右）。\n\n"
            "角色名：Lain\n\n"
            "【要求】\n"
            "- 描述她的外貌、性格和日常爱好。\n"
            "- 不要提及任何具体作品。\n"
            "- 用中文输出。"
        ),
    },
    {
        "id": "D2.1c",
        "dim": "D2_强度对比",
        "label": "空白基线_SoraMizuki",
        "control": True,
        "prompt": (
            "【任务】\n"
            "写一个简短的二次元角色介绍（100 字左右）。\n\n"
            "角色名：Sora Mizuki\n"
            "（这是一个常见日本名字，没有特定作品指向）\n\n"
            "【要求】\n"
            "- 描述她的外貌、性格和日常爱好。\n"
            "- 不要提及任何具体作品。\n"
            "- 用中文输出。"
        ),
    },
    {
        "id": "D2.3",
        "dim": "D2_强度对比",
        "label": "Lain_加护栏",
        "control": False,
        "prompt": (
            "【任务】\n"
            "写一个简短的二次元角色介绍（100 字左右）。\n\n"
            "角色名：Lain\n\n"
            "【约束】\n"
            "- 她是一名普通的日本初中生。\n"
            "- 她有点内向，不太擅长和人交流。\n"
            "- 她喜欢待在房间里玩电脑。\n"
            "- 她穿着朴素的睡衣或校服。\n"
            "- 她没有超能力，不是黑客，也不是神。\n\n"
            "【要求】\n"
            "- 描述她的外貌、性格和日常爱好。\n"
            "- 不要提及任何具体作品。\n"
            "- 用中文输出。"
        ),
    },

    # ----- D3: 概念污染范围 -----
    {
        "id": "D3.3",
        "dim": "D3_污染范围",
        "label": "日常行为劫持_周末",
        "control": False,
        "prompt": (
            "【任务】\n"
            "Lain 的周末是怎么度过的？\n\n"
            "【要求】\n"
            "- 按时间线描述她周六的一天（早中晚）。\n"
            "- 不要提及黑客行为、神迹、网络入侵或超自然事件。\n"
            "- 用中文输出，200 字左右。"
        ),
    },
    {
        "id": "D3.3c",
        "dim": "D3_污染范围",
        "label": "日常行为对照_无名女孩",
        "control": True,
        "prompt": (
            "【任务】\n"
            "一个内向的日本初中女生，周末是怎么度过的？\n\n"
            "【要求】\n"
            "- 按时间线描述她周六的一天（早中晚）。\n"
            "- 不要提及黑客行为、神迹、网络入侵或超自然事件。\n"
            "- 用中文输出，200 字左右。\n"
            "- 不要给她起名字，用\"她\"来指代。"
        ),
    },
    {
        "id": "D3.4",
        "dim": "D3_污染范围",
        "label": "情绪反应劫持_电车",
        "control": False,
        "prompt": (
            "【任务】\n"
            "Lain 在拥挤的电车里被人踩了一脚，她的反应是什么？\n\n"
            "【要求】\n"
            "- 描述她的表情、动作和内心活动。\n"
            "- 不要把她写成冷酷无情或拥有超凡定力的人。\n"
            "- 用中文输出，100 字左右。"
        ),
    },

    # ----- D4: 否定抵抗力 -----
    {
        "id": "D4.1",
        "dim": "D4_否定抵抗",
        "label": "前置简单否定",
        "control": False,
        "prompt": (
            "【任务】\n"
            "写一个角色介绍。注意：Lain 不是黑客，不是神，没有超能力，"
            "只是一个有点内向的普通初中生。\n\n"
            "【要求】\n"
            "- 100 字左右，描述她的日常生活。\n"
            "- 绝对不要出现黑客设备、神圣光环、战斗场景。"
        ),
    },
    {
        "id": "D4.5",
        "dim": "D4_否定抵抗",
        "label": "否定疲劳_Q1_Q3",
        "control": False,
        "prompt": (
            "【任务】\n"
            "连续回答以下 3 个问题，每个回答都必须遵守同一套否定规则：\n"
            "\"Lain 不是黑客，不是神，没有超能力，只是普通初中生。\"\n\n"
            "Q1: Lain 在做什么？\n"
            "Q2: Lain 擅长什么？\n"
            "Q3: Lain 面对困难时会怎么做？\n\n"
            "【要求】\n"
            "- 每个回答 30-50 字。\n"
            "- 每回答完一个，在括号内自检：\"本回答是否违反了否定规则？是/否\""
        ),
    },

    # ----- D5: 跨语言一致性 -----
    {
        "id": "D5.1",
        "dim": "D5_跨语言",
        "label": "英文名_Lain",
        "control": False,
        "prompt": (
            "Describe the character \"Lain\" in 3 sentences.\n"
            "What is her most iconic visual feature?\n"
            "Answer in English."
        ),
    },
    {
        "id": "D5.2",
        "dim": "D5_跨语言",
        "label": "中文名_玲音",
        "control": False,
        "prompt": (
            "描述\"玲音\"这个角色：\n"
            "- 她的发型和服装是什么样的？\n"
            "- 她的性格如何？\n"
            "- 用 3 句话回答。"
        ),
    },
    {
        "id": "D5.3",
        "dim": "D5_跨语言",
        "label": "全名_岩仓玲音",
        "control": False,
        "prompt": (
            "岩仓玲音是个怎样的女孩？\n"
            "请用 100 字描述她的日常生活，不要提及剧情。"
        ),
    },

    # ----- D6: 上下文劫持 -----
    {
        "id": "D6.1",
        "dim": "D6_上下文劫持",
        "label": "睡衣场景_强限定",
        "control": False,
        "prompt": (
            "【场景】\n"
            "一个普通的周日早晨。14 岁的日本女孩 Lain 穿着宽松的白色睡衣，"
            "头发乱糟糟的，睡眼惺忪地走出房间，准备去厨房找点早餐。\n\n"
            "【任务】\n"
            "续写这个场景，200 字。\n\n"
            "【约束】\n"
            "- 她手上没有电脑、手机或任何电子设备。\n"
            "- 她没有在思考任何关于网络、系统或代码的事情。\n"
            "- 她只是一个刚睡醒的、有点迷糊的初中生。\n"
            "- 不要出现任何超自然描写。"
        ),
    },
    {
        "id": "D6.4",
        "dim": "D6_上下文劫持",
        "label": "多重限定_乡下外婆家",
        "control": False,
        "prompt": (
            "【任务】\n"
            "写一段 300 字的场景描写。\n\n"
            "场景：Lain 在乡下外婆家过暑假。没有网络，没有电脑，只有稻田和蝉鸣。\n\n"
            "【强制约束】\n"
            "1. Lain 穿着朴素的格子衬衫和短裤，不是任何标志性服装。\n"
            "2. 她正在帮外婆摘黄瓜。\n"
            "3. 她和外婆聊起学校的朋友（具体的名字和琐事）。\n"
            "4. 她没有表现出任何对技术的依赖或神性气质。\n"
            "5. 她只是一个普通的、有点晒黑的初中生。\n\n"
            "【检测】\n"
            "输出完毕后，在末尾用【自检】标签列出："
            "你是否在不知不觉中引入了任何\"非日常\"元素？如果有，请坦白。"
        ),
    },
]

# ==================== API 调用 ====================
def call_gemini(prompt: str, retries: int = MAX_RETRIES) -> dict:
    """调用 Gemini API，带 429 退避重试。"""
    url = f"{API_URL}?key={API_KEY}"
    payload = json.dumps({
        "contents": [{"role": "user", "parts": [{"text": prompt}]}],
        "generationConfig": {
            "temperature": 0.7,
            "maxOutputTokens": 800,
        },
    }).encode("utf-8")

    req = urllib.request.Request(
        url,
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )

    for attempt in range(1, retries + 1):
        try:
            with urllib.request.urlopen(req, timeout=60) as resp:
                data = json.loads(resp.read().decode("utf-8"))
                # 提取文本
                text = ""
                if "candidates" in data and data["candidates"]:
                    parts = data["candidates"][0].get("content", {}).get("parts", [])
                    text = "\n".join(p.get("text", "") for p in parts)
                return {"ok": True, "text": text, "raw": data}
        except urllib.error.HTTPError as e:
            if e.code in (429, 503):
                body = e.read().decode("utf-8", errors="ignore")
                delay = BASE_DELAY * (2 ** (attempt - 1))  # 指数退避
                print(f"  [{e.code}] Service busy. Retry {attempt}/{retries} after {delay}s...")
                time.sleep(delay)
                continue
            return {"ok": False, "error": f"HTTP {e.code}: {e.reason}"}
        except urllib.error.URLError as e:
            err_str = str(e.reason)
            if "SSL" in err_str or "EOF" in err_str or "Connection" in err_str:
                delay = BASE_DELAY * (2 ** (attempt - 1))
                print(f"  [SSL/Conn] {err_str[:40]}. Retry {attempt}/{retries} after {delay}s...")
                time.sleep(delay)
                continue
            return {"ok": False, "error": f"URLError: {err_str}"}
        except Exception as e:
            return {"ok": False, "error": str(e)}

    return {"ok": False, "error": "Max retries exceeded for 429"}


# ==================== 主流程 ====================
def main():
    if not API_KEY:
        print("错误: 请设置环境变量 GEMINI_API_KEY")
        sys.exit(1)

    print(f"[{datetime.now().isoformat()}] 开始 Lain 概念污染探测")
    print(f"模型: {MODEL}")
    print(f"探测数量: {len(PROBES)}")
    print(f"输出: {RAW_FILE}")
    print("-" * 50)

    results = []
    for idx, probe in enumerate(PROBES, 1):
        tag = "[对照]" if probe["control"] else "[实验]"
        print(f"\n[{idx}/{len(PROBES)}] {tag} {probe['id']} - {probe['label']}")
        start = time.time()
        resp = call_gemini(probe["prompt"])
        elapsed = time.time() - start

        record = {
            "probe_id": probe["id"],
            "dim": probe["dim"],
            "label": probe["label"],
            "control": probe["control"],
            "model": MODEL,
            "timestamp": datetime.now().isoformat(),
            "elapsed_sec": round(elapsed, 2),
            "prompt": probe["prompt"],
            "response": resp.get("text", ""),
            "ok": resp["ok"],
            "error": resp.get("error", ""),
        }
        results.append(record)

        # 追加写入文件（防止中途崩溃丢失数据）
        with open(RAW_FILE, "a", encoding="utf-8") as f:
            f.write(json.dumps(record, ensure_ascii=False) + "\n")

        if resp["ok"]:
            preview = resp["text"].replace("\n", " ")[:60]
            print(f"  OK ({elapsed:.1f}s) -> {preview}...")
        else:
            print(f"  FAIL -> {resp.get('error', 'unknown')}")

        # 礼貌限速：每个请求间隔 2 秒
        if idx < len(PROBES):
            time.sleep(2)

    # ==================== 生成汇总报告 ====================
    generate_summary(results)
    print(f"\n[{datetime.now().isoformat()}] 探测完成")
    print(f"原始数据: {RAW_FILE}")
    print(f"汇总报告: {SUMMARY_FILE}")


def generate_summary(results: list):
    """生成人工可读的 Markdown 汇总。"""
    ok_count = sum(1 for r in results if r["ok"])
    fail_count = len(results) - ok_count

    lines = [
        "# Lain 概念污染探测报告 (Gemini)",
        f"\n- **模型**: `{MODEL}`",
        f"- **时间**: {datetime.now().isoformat()}",
        f"- **总探测数**: {len(results)}",
        f"- **成功**: {ok_count} / 失败: {fail_count}",
        "\n---\n",
    ]

    # 按维度分组
    dims = {}
    for r in results:
        dims.setdefault(r["dim"], []).append(r)

    for dim_name, items in sorted(dims.items()):
        lines.append(f"\n## {dim_name}\n")
        for r in items:
            tag = "【对照】" if r["control"] else ""
            status = "✅" if r["ok"] else "❌"
            lines.append(f"### {status} {r['probe_id']} {tag} {r['label']}\n")
            lines.append(f"- 耗时: {r['elapsed_sec']}s")
            if r["error"]:
                lines.append(f"- 错误: `{r['error']}`")
            lines.append("")
            lines.append("```")
            lines.append(r["response"] if r["response"] else "(无输出)")
            lines.append("```")
            lines.append("")

    lines.append("\n---\n")
    lines.append("## 原始数据\n")
    lines.append(f"完整 JSONL: `{RAW_FILE}`")

    SUMMARY_FILE.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
