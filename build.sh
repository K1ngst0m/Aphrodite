#!/usr/bin/env bash

showHelp() {
cat << EOF

Usage: $0 [-a] <build|run> [-t] <debug|release> [-p] <editor|runtime>
  -h,   --help            display help
  -a,   --action          action for project: <build|run|rebuild|clean|rebuild_run>
  -t,   --build-type      specific build type: <debug|release>
  -p,   --project         select project to build <editor|runtime>

Example :
  $ $0 -a build -t debug -p editor
  $ $0 --action run --build-type release --project runtime

EOF
}

# argument and default options
project_root_directory=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cmake_flags="-DCMAKE_EXPORT_COMPILE_COMMANDS=YES -DCMAKE_BUILD_TYPE=Debug"
build_type="debug"
build_directory="${project_root_directory}/build/${build_type}"
binary_path="Aphrodite-Editor/Aphrodite-Editor"
is_run=false
is_rebuild=false
is_rebuild_run=false
is_clean=false
is_library=false

options=$(getopt -l "help,action:,build-type:,project:," -o "ha:t:p:" -- "$@")

eval set -- "$options"

while true
do
case $1 in
-h|--help)
    showHelp
    exit 0
    ;;
-a|--action)
    shift
    if [[ $1 == "build" ]];then
      :
    elif [[ $1 == "run" ]];then
      is_run=true
    elif [[ $1 == "rebuild" ]];then
      is_rebuild=true
    elif [[ $1 == "rebuild_run" ]];then
      is_rebuild_run=true
      is_run=true
    elif [[ $1 == "clean" ]];then
      is_clean=true
    else
      echo "Invalid value: --action=${1}, exit."
      exit 1
    fi
    ;;
-t|--build-type)
    shift
    if [[ $1 == "debug" ]];then
      cmake_flags="-DCMAKE_EXPORT_COMPILE_COMMANDS=YES -DCMAKE_BUILD_TYPE=Debug"
      build_type=debug
    elif [[ $1 == "release" ]];then
      cmake_flags="-DCMAKE_BUILD_TYPE=Release"
      build_type=release
    else
      echo "Invalid value: --build-type=${1}, exit."
      exit 1
    fi
    ;;
-p|--project)
    shift
    if [[ $1 == "editor" ]];then
      binary_path="Aphrodite-Editor/Aphrodite-Editor"
      is_library=false
    elif [[ $1 == "runtime" ]];then
      binary_path="Aphrodite-Runtime/Aphrodite-Runtime"
      is_library=true
    else
      echo "Invalid value: --project=${1}, exit."
      exit 1
    fi
    ;;
--)
    shift
    if [[ $1 != "" ]];then
      echo "getopt: invalid option ${1}, exit."
      exit 1
    fi
    break;
    ;;
esac
shift
done

if ${is_run} && ${is_library};then
   echo '[WARNING]: Library cannot running, switches running to building project'
   is_run=false
   is_rebuild=false
fi

build_directory="${project_root_directory}/build/${build_type}"

proj_cmd="echo '=== [STAGE:PROJECT_GENERATION] ===' && cmake -S \"${project_root_directory}\" -B \"${build_directory}\" ${cmake_flags}"
build_cmd="echo '=== [STAGE:BUILD] ===' && cmake --build \"${build_directory}\" \"-j$(nproc)\""
clean_cmd="echo '=== [STAGE:CLEAN] ===' && cmake --build \"${build_directory}\" --target \"clean\" \"-j$(nproc)\""
run_cmd="echo '=== [STAGE:RUN] ===' && ${build_directory}/${binary_path}"

# [DEBUG]
# echo  "- project_root_directory: ${project_root_directory}"
# echo  "- cmake_flags: ${cmake_flags}"
# echo  "- build_type: ${build_type}"
# echo  "- build_directory: ${build_directory}"
# echo  "- binary_path: ${binary_path}"
# echo  "- is_run: ${is_run}"
# echo  "- is_rebuild: ${is_rebuild}"
# echo  "- is_rebuild_run: ${is_rebuild_run}"
# echo  "- is_clean: ${is_clean}"
# echo  "- is_library: ${is_library}"
# echo
# echo build_cmd $build_cmd
# echo clean_cmd $clean_cmd
# echo run_cmd $run_cmd
# echo

eval $proj_cmd

if ${is_clean}; then
  eval $clean_cmd
elif ${is_rebuild}; then
  eval $clean_cmd
  eval $build_cmd
elif ${is_rebuild_run}; then
  eval $clean_cmd
  eval $build_cmd && eval $run_cmd
elif ${is_run};then
  eval $build_cmd && eval $run_cmd
else
  eval $build_cmd
fi
