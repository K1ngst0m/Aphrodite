#!/usr/bin/env bash
#

shader_src_list=$(find -L data -iname '*.vert' -o -iname '*.frag')

for shader_src in $shader_src_list
do
    rpath=$(realpath $shader_src)
    dir="$(dirname -- "$rpath")"
    name="$(basename -- "$rpath")"
    glslc "$rpath" -o "$dir/$name.spv"
done
