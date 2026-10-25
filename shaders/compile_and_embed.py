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

# 1. Læs den originale GLSL kildekode til string_view-brug senere
try:
    with open(input_glsl, "r", encoding="utf-8") as f:
        glsl_source_text = f.read()
except Exception as e:
    print(f"Error: Could not read source file {input_glsl}: {e}", file=sys.stderr)
    sys.exit(1)

# Definer en midlertidig .spv fil i samme mappe som output-headeren
temp_spv = output_header + ".tmp.spv"

try:
    # 2. Kør glslc på den læsbare GLSL kildekode
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

# 3. Læs den genererede binære SPIR-V data
try:
    with open(temp_spv, "rb") as f:
        bytes_data = f.read()
finally:
    # Ryd op og slet den midlertidige .spv fil med det samme
    if os.path.exists(temp_spv):
        os.remove(temp_spv)

# SPIR-V data skal være aligned til 4 bytes (uint32_t)
if len(bytes_data) % 4 != 0:
    print(f"Error: SPIR-V binary size for {input_glsl} is not a multiple of 4", file=sys.stderr)
    sys.exit(1)

# Konverter bytes til et array af 32-bit usignerede heltal (uint32_t)
words_count = len(bytes_data) // 4
words = struct.unpack(f"<{words_count}I", bytes_data)

# 4. Skriv C++ header-filen med std::array og std::string_view
with open(output_header, "w", encoding="utf-8") as f:
    f.write("#pragma once\n")
    f.write("#include <array>\n")
    f.write("#include <cstdint>\n")
    f.write("#include <string_view>\n\n")
    
    # Namespace/Scope beskyttelse (valgfrit men god stil)
    f.write(f"namespace embedded_shaders::{var_name.lower()} {{\n\n")
    
    # Skriv SPIR-V binær data som std::array
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
    
    # Skriv original GLSL kildekode som std::string_view via C++ Raw String Literal R"glsl(...)glsl"
    f.write("inline constexpr std::string_view source = R\"glsl(\n")
    f.write(glsl_source_text)
    # Sørg for at tilføje et linjeskift hvis kildekoden mangler et i bunden, så lukke-tagget står for sig selv
    if not glsl_source_text.endswith('\n'):
        f.write("\n")
    f.write(")glsl\";\n\n")
    
    f.write("}\n")

