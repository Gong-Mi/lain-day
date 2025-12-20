import sys
import os

def main():
    input_path = "experiments/ansi_renderer/e.jpeg.txt"
    output_path = "include/logo_data.h"
    
    if not os.path.exists(input_path):
        print(f"Error: {input_path} not found.")
        return

    try:
        with open(input_path, "r", encoding='utf-8') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"Error reading input file: {e}")
        return

    c_lines = []
    for line in lines:
        # Escape backslashes and double quotes
        # We need to be careful with hex escape sequences in the source not being interpreted by C compiler 
        # if they form invalid sequences, but here we just have raw text that happens to be ANSI.
        # The Python string will contain the actual bytes. We need to represent them in C.
        
        # Actually, simpler approach: represent as hex array to avoid encoding issues?
        # No, ANSI works fine as string if we escape correctly.
        
        escaped_line = ""
        for char in line:
            if char == '\\':
                escaped_line += "\\\\"
            elif char == '"':
                escaped_line += "\\"
            elif char == '\n':
                escaped_line += "\\n"
            elif char == '\r':
                pass # Ignore carriage returns
            elif 32 <= ord(char) <= 126:
                escaped_line += char
            else:
                # Use hex representation for non-printable chars (like ESC \x1b)
                escaped_line += f"\\x{ord(char):02x}"

        # Splitting long lines is good practice for C compilers
        c_lines.append(f"    \"{escaped_line}\"")

    header_content = "#ifndef LOGO_DATA_H\n#define LOGO_DATA_H\n\nconst char* GAME_LOGO = \n" + "\n".join(c_lines) + ";\n\n#endif // LOGO_DATA_H\n"

    try:
        with open(output_path, "w", encoding='utf-8') as f:
            f.write(header_content)
        print(f"Generated {output_path}")
    except Exception as e:
        print(f"Error writing output file: {e}")

if __name__ == "__main__":
    main()
