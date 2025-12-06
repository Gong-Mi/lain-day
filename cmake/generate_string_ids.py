import json
import sys
import os

def generate_string_ids_h(json_path, header_path, names_header_path, names_c_path):
    try:
        with open(json_path, 'r', encoding='utf-8') as f:
            string_data = json.load(f)
    except FileNotFoundError:
        print(f"Error: JSON file not found at {json_path}", file=sys.stderr)
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error: Malformed JSON in {json_path}: {e}", file=sys.stderr)
        sys.exit(1)

    ids = sorted(string_data.keys())
    
    # Check for empty IDs or non-string IDs
    for _id in ids:
        if not _id or not isinstance(_id, str):
            print(f"Error: Invalid (empty or non-string) ID found in {json_path}", file=sys.stderr)
            sys.exit(1)

    # Ensure output directories exist
    os.makedirs(os.path.dirname(header_path), exist_ok=True)
    os.makedirs(os.path.dirname(names_header_path), exist_ok=True)
    os.makedirs(os.path.dirname(names_c_path), exist_ok=True)


    # Generate string_ids.h
    with open(header_path, 'w', encoding='utf-8') as f:
        f.write("#ifndef STRING_IDS_H\n")
        f.write("#define STRING_IDS_H\n\n")
        f.write("typedef enum {\n")
        f.write("    TEXT_INVALID = 0,\n") # Always keep 0 for invalid
        f.write("    TEXT_EMPTY_LINE,\n")  # Ensure empty line also gets an ID
        generated_ids_count = 2 # Start from 2 as 0 and 1 are hardcoded
        for _id in ids:
            # Skip TEXT_INVALID and TEXT_EMPTY_LINE if they are in JSON, as they are hardcoded here
            if _id not in ["TEXT_INVALID", "TEXT_EMPTY_LINE"]:
                f.write(f"    {_id} = {generated_ids_count},\n")
                generated_ids_count += 1
        f.write(f"    TEXT_COUNT // Total number of strings, which is {generated_ids_count}\n")
        f.write("} StringID;\n\n")
        f.write("#endif // STRING_IDS_H\n")

    print(f"Generated {header_path} with {generated_ids_count} string IDs.")

    # Generate string_id_names.h (declaration)
    with open(names_header_path, 'w', encoding='utf-8') as f:
        f.write("#ifndef STRING_ID_NAMES_H\n")
        f.write("#define STRING_ID_NAMES_H\n\n")
        f.write("#include \"string_ids.h\"\n\n") # Include generated string_ids.h
        f.write("extern const char* g_string_id_names[TEXT_COUNT];\n\n") # extern declaration
        f.write("#endif // STRING_ID_NAMES_H\n")

    print(f"Generated {names_header_path} (declaration).")

    # Generate generated_string_id_names.c (definition)
    with open(names_c_path, 'w', encoding='utf-8') as f:
        f.write("#include \"string_id_names.h\"\n") # Include its own declaration
        f.write("#include \"string_ids.h\"\n\n") # Also need StringID enum values

        f.write("const char* g_string_id_names[TEXT_COUNT] = {\n")
        
        # Ensure TEXT_INVALID and TEXT_EMPTY_LINE are at their correct hardcoded indices
        f.write(f"    [TEXT_INVALID] = \"TEXT_INVALID\",\n")
        f.write(f"    [TEXT_EMPTY_LINE] = \"TEXT_EMPTY_LINE\",\n")

        # Dynamically add other IDs based on their assigned enum value
        for _id in ids:
            if _id not in ["TEXT_INVALID", "TEXT_EMPTY_LINE"]:
                f.write(f"    [{_id}] = \"{_id}\",\n")
        f.write("};\n\n")

    print(f"Generated {names_c_path} (definition).")


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("Usage: python generate_string_ids_h.py <path_to_json> <path_to_ids_header_file> <path_to_names_header_file> <path_to_names_c_file>", file=sys.stderr)
        sys.exit(1)

    json_input_path = sys.argv[1]
    header_output_path = sys.argv[2]
    names_header_output_path = sys.argv[3]
    names_c_output_path = sys.argv[4]
    generate_string_ids_h(json_input_path, header_output_path, names_header_output_path, names_c_output_path)