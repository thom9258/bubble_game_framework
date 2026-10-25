#!/bin/bash

SOURCE_DIR="../shaders"
OUTPUT_DIR="build/Debug"

[[ -d ${OUTPUT_DIR} ]] || mkdir ${OUTPUT_DIR}

function compile_vert()
{
  glslc "$1".vert -o ${OUTPUT_DIR}/"$1".vert.spv
  echo "compiled ${SOURCE_DIR}/""$1"".vert to ${OUTPUT_DIR}/""$1"".vert.spv"
}

function compile_vert_frag ()
{
  glslc "${SOURCE_DIR}/$1.vert" -o "${OUTPUT_DIR}/$1.vert.spv"
  glslc "${SOURCE_DIR}/$1.frag" -o "${OUTPUT_DIR}/$1.frag.spv"
  echo "compiled ${OUTPUT_DIR}/$1.vert.spv & ${OUTPUT_DIR}/$1.frag.spv"
}

compile_vert_frag "flatcolor"
compile_vert_frag "texture"
