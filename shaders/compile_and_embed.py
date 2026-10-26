#!/usr/bin/env python3
import sys
import os
import subprocess
import struct

if len(sys.argv) < 5:
    print("Usage: compile_and_embed.py <glslc_path> <input_glsl> <output_header> <variable_name>")
    sys.exit(1)

glslc_path = sys.argv[1]
input_glsl = sys.argv[2]
output_header = sys.argv[3]
var_name = sys.argv[4]

try:
    with open(input_glsl, "r", encoding="utf-8") as f:
        glsl_source_text = f.read()
except Exception as e:
    print(f"Error: Could not read source file {input_glsl}: {e}", file=sys.stderr)
    sys.exit(1)

temp_spv = output_header + ".tmp.spv"

try:
    result = subprocess.run(
        [glslc_path, input_glsl, "-o", temp_spv],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
except subprocess.CalledProcessError as e:
    print(f"Error: glslc failed to compile {input_glsl}:", file=sys.stderr)
    print(e.stderr, file=sys.stderr)
    sys.exit(1)
except FileNotFoundError:
    print(f"Error: glslc executable not found at '{glslc_path}'", file=sys.stderr)
    sys.exit(1)

try:
    with open(temp_spv, "rb") as f:
        bytes_data = f.read()
finally:
    if os.path.exists(temp_spv):
        os.remove(temp_spv)

if len(bytes_data) % 4 != 0:
    print(f"Error: SPIR-V binary size for {input_glsl} is not a multiple of 4", file=sys.stderr)
    sys.exit(1)

words_count = len(bytes_data) // 4
words = struct.unpack(f"<{words_count}I", bytes_data)

with open(output_header, "w", encoding="utf-8") as f:
    f.write("#pragma once\n")
    f.write("#include <array>\n")
    f.write("#include <cstdint>\n")
    f.write("#include <string_view>\n\n")
    f.write(f"namespace embedded_shaders::{var_name.lower()} {{\n\n")
    f.write(f"inline constexpr std::array<std::uint32_t, {words_count}> spirv = {{\n")
    for i, word in enumerate(words):
        if i % 6 == 0:
            f.write("  ")
        f.write(f"0x{word:08X}, ")
        if (i + 1) % 6 == 0:
            f.write("\n")
    if words_count % 6 != 0:
        f.write("\n")
    f.write("};\n\n")
    
    f.write("inline constexpr std::string_view source = R\"glsl(\n")
    f.write(glsl_source_text)
    if not glsl_source_text.endswith('\n'):
        f.write("\n")
    f.write(")glsl\";\n\n")
    
    f.write("}\n")

