import sys
import struct
from PIL import Image

def main(image_path, output_path, max_width=400):
    try:
        img = Image.open(image_path).convert("RGB")
    except Exception as e:
        print(f"Error: {e}")
        return

    width, height = img.size

    # --- Add Pre-scaling Logic ---
    if width > max_width:
        aspect_ratio = height / width
        new_width = max_width
        new_height = int(new_width * aspect_ratio)
        img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
        print(f"Image resized from {width}x{height} to {new_width}x{new_height}.")
        width = new_width
        height = new_height
    # --- End Pre-scaling Logic ---

    pixels = list(img.getdata())

    print(f"Converting {image_path} ({width}x{height}) to {output_path}...")

    with open(output_path, "wb") as f:
        # Write Header: Width (4 bytes), Height (4 bytes)
        f.write(struct.pack("II", width, height))
        
        # Write Pixel Data (R, G, B bytes)
        # Using a flat byte array is faster
        data = bytearray()
        for r, g, b in pixels:
            data.append(r)
            data.append(g)
            data.append(b)
        f.write(data)

    print("Done.")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 jpg_to_raw.py <input> <output> [max_width]")
    else:
        path = sys.argv[1]
        out_path = sys.argv[2]
        m_width = int(sys.argv[3]) if len(sys.argv) > 3 else 400
        main(path, out_path, m_width)