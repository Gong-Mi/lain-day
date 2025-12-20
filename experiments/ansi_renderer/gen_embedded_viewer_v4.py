import sys
from PIL import Image

def generate_c_viewer(image_path, template_path, output_c_path, target_width=200):
    try:
        img = Image.open(image_path).convert("RGB")
    except Exception as e:
        print(f"Error opening image: {e}")
        sys.exit(1)

    # 1. Resize Image
    w, h = img.size
    ratio = h / w
    new_w = target_width
    new_h = int(new_w * ratio)
    
    img = img.resize((new_w, new_h), Image.Resampling.LANCZOS)
    data = list(img.getdata())

    print(f"Image resized to {new_w}x{new_h} for embedding.")

    # 2. Generate C Array String
    # We use a list and join for efficiency
    c_array_parts = []
    for i, pixel in enumerate(data):
        c_array_parts.append(f"0x{pixel[0]:02X},0x{pixel[1]:02X},0x{pixel[2]:02X}")
    
    c_array_str = ",".join(c_array_parts)

    # 3. Read Template
    try:
        with open(template_path, 'r', encoding='utf-8') as f:
            template_content = f.read()
    except Exception as e:
        print(f"Error reading template: {e}")
        sys.exit(1)
    
    # 4. Replace Placeholders
    final_source = template_content.replace("__WIDTH__", str(new_w)) \
                                   .replace("__HEIGHT__", str(new_h)) \
                                   .replace("__DATA__", c_array_str)

    # 5. Write Output
    with open(output_c_path, 'w', encoding='utf-8') as f:
        f.write(final_source)
    
    print(f"Successfully generated {output_c_path}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python gen.py <img_path> <template_path> <out_c_path>")
        sys.exit(1)
    
    # image path, template path, output path
    generate_c_viewer(sys.argv[1], sys.argv[2], sys.argv[3])
