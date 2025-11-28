import re
import sys

def bake_art(filename, output_bin="art.bin"):
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"读取失败: {e}")
        return

    tokens = re.split(r'(\x1b\[[0-9;]*m|\n)', content)
    
    # 状态追踪
    cur_fg = "255;255;255"
    cur_bg = "0;0;0"
    last_fg = ""
    last_bg = ""
    
    # 二进制缓冲区
    blob = bytearray()
    
    # 隐藏光标
    blob.extend(b"\x1b[?25l")

    for token in tokens:
        if not token: continue
        
        if token == '\n':
            # 换行时重置背景，防止溢出，然后换行
            blob.extend(b"\x1b[0m\n")
            # 换行后状态失效
            last_fg = ""
            last_bg = ""
            continue

        if token.startswith('\x1b['):
            # 解析颜色（逻辑同前，略微简化）
            code = token[2:-1]
            parts = code.split(';')
            i = 0
            temp_fg = cur_fg
            temp_bg = cur_bg
            
            while i < len(parts):
                if parts[i] == '38' and i+4 < len(parts):
                    temp_fg = f"{parts[i+2]};{parts[i+3]};{parts[i+4]}"
                    i += 5
                elif parts[i] == '48' and i+4 < len(parts):
                    temp_bg = f"{parts[i+2]};{parts[i+3]};{parts[i+4]}"
                    i += 5
                elif parts[i] == '0':
                    temp_fg = "255;255;255"
                    temp_bg = "0;0;0"
                    i += 1
                else:
                    i += 1
            
            # 关键优化：这里只更新状态变量，不写入数据
            # 数据在下面写入字符时才根据状态差异写入
            cur_fg = temp_fg
            cur_bg = temp_bg

        else:
            # 遇到字符了，检查颜色是否需要更新
            if cur_fg != last_fg:
                # 写入前景色指令
                cmd = f"\x1b[38;2;{cur_fg}m"
                blob.extend(cmd.encode('utf-8'))
                last_fg = cur_fg
            
            if cur_bg != last_bg:
                # 写入背景色指令
                cmd = f"\x1b[48;2;{cur_bg}m"
                blob.extend(cmd.encode('utf-8'))
                last_bg = cur_bg
            
            # 写入字符本身 (UTF-8)
            blob.extend(token.encode('utf-8'))

    # 恢复光标
    blob.extend(b"\x1b[0m\x1b[?25h\n")

    with open(output_bin, 'wb') as f:
        f.write(blob)
    
    print(f"烘焙完成！生成了 {output_bin}。")
    print(f"现在你可以直接用 'cat {output_bin}' 来显示，或者用极速 C 程序读取它。")

if __name__ == "__main__":
    bake_art("d")