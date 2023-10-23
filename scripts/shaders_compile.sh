#!/usr/bin/env bash
#

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

shader_src_list=$(find ${SCRIPT_DIR}/.. -L assets -iname '*.vert' -o -iname '*.frag' -o -iname '*.comp')

for shader_src in $shader_src_list
do
    rpath=$(realpath "$shader_src")
    dir="$(dirname -- "$rpath")"
    name="$(basename -- "$rpath")"
    # glslc "$rpath" -o "$dir/$name.spv"
    glslangValidator --quiet --target-env vulkan1.3 "$rpath" -o "$dir/$name.spv"&
done
