import sys
import time
import os

def typewriter_print(text, delay=0.04):
    for char in text:
        sys.stdout.write(char)
        sys.stdout.flush()
        time.sleep(delay)

def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

def render_scene(story_content, available_choices):
    clear_screen()
    for item in story_content:
        if isinstance(item, tuple) and item[0] == 'pause':
            time.sleep(item[1])
        elif isinstance(item, str):
            typewriter_print(item)
    
    print("\n" + "="*30 + "\n")

    if not available_choices:
        print("...路径似乎在这里中断了。")
        # The exit input is handled in the main loop now
        return

    for i, choice in enumerate(available_choices):
        if "(无法选择)" in choice['text']:
            print(f"  {i + 1}. {choice['text']}")
        else:
            print(f"{i + 1}. {choice['text']}")
