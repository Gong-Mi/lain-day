import sys
from PIL import Image

# ASCII characters by density
ASCII_CHARS = ["@", "#", "S", "%", "?", "*", "+", ";", ":", ",", "."]

def resize_image(image, new_width=100):
    width, height = image.size
    ratio = height / width / 1.65  # Adjust aspect ratio for terminal fonts
    new_height = int(new_width * ratio)
    return image.resize((new_width, new_height))

def pixel_to_ansi(pixel):
    r, g, b = pixel
    # ANSI RGB foreground color escape code
    return f"\x1b[38;2;{r};{g};{b}m"

def main(image_path, output_path, width=80):
    try:
        image = Image.open(image_path).convert("RGB")
    except Exception as e:
        print(f"Unable to open image: {e}")
        return

    image = resize_image(image, new_width=width)
    pixels = image.getdata()
    
    # Simple brightness-based character selection
    # (Optional: map brightness to ASCII_CHARS if we want actual characters, 
    # but for pure image display, block characters like '█' might be better, 
    # or just use background colors.)
    
    # Method 1: Foreground colored characters (using a solid block)
    # ascii_str = ""
    # for pixel in pixels:
    #     ascii_str += pixel_to_ansi(pixel) + "█"
    #     if (len(ascii_str) // 20) % image.width == 0: # This logic is flawed for string len
    #         pass 

    # Correct Loop
    width, height = image.size
    with open(output_path, 'w', encoding='utf-8') as f:
        for y in range(height):
            for x in range(width):
                r, g, b = image.getpixel((x, y))
                # Using two spaces for aspect ratio correction + background color
                # This often looks better for "images" in terminal
                f.write(f"\x1b[48;2;{r};{g};{b}m  ")
            f.write("\x1b[0m\n") # Reset at EOL

    print(f"Successfully converted {image_path} to ANSI file {output_path}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 img_to_ansi.py <input_image> [width]")
    else:
        path = sys.argv[1]
        w = int(sys.argv[2]) if len(sys.argv) > 2 else 100
        out = path + ".txt"
        main(path, out, w)
