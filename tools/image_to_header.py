import sys
from PIL import Image
import os

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 image_to_header.py <input_image> <output_header>")
        return

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    
    if not os.path.exists(input_path):
        print(f"Error: {input_path} not found.")
        return

    try:
        image = Image.open(input_path).convert("RGB")
    except Exception as e:
        print(f"Error opening image: {e}")
        return

    width, height = image.size
    pixels = list(image.getdata())

    header_guard = os.path.basename(output_path).replace(".", "_").upper()
    
    with open(output_path, "w") as f:
        f.write(f"#ifndef {header_guard}\n")
        f.write(f"#define {header_guard}\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"static const int LOGO_WIDTH = {width};\n")
        f.write(f"static const int LOGO_HEIGHT = {height};\n")
        f.write("static const uint8_t LOGO_DATA[] = {\n")
        
        for i, pixel in enumerate(pixels):
            r, g, b = pixel
            f.write(f"    {r}, {g}, {b},")
            if (i + 1) % 10 == 0:
                f.write("\n")
        
        f.write("\n};\n\n")
        f.write(f"#endif // {header_guard}\n")

    print(f"Generated {output_path}")

if __name__ == "__main__":
    main()
