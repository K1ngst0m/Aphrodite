#!/usr/bin/env python3

import os
import sys
import subprocess
from typing import List, Set
import json

def find_cmake_files(root_dir: str) -> Set[str]:
    """Find all CMake files in the project."""
    cmake_files = set()
    for root, _, files in os.walk(root_dir):
        for file in files:
            if file in ['CMakeLists.txt', '*.cmake']:
                cmake_files.add(os.path.join(root, file))
    return cmake_files

def check_cmake_format_installed() -> bool:
    """Check if cmake-format is installed."""
    try:
        subprocess.run(['cmake-format', '--version'], capture_output=True, check=True)
        return True
    except subprocess.CalledProcessError:
        return False

def format_file(file_path: str, check_only: bool = False) -> bool:
    """Format a single CMake file."""
    try:
        if check_only:
            result = subprocess.run(
                ['cmake-format', '--check', file_path],
                capture_output=True,
                text=True
            )
            if result.returncode != 0:
                print(f"❌ {file_path} needs formatting")
                return False
            print(f"✅ {file_path} is properly formatted")
            return True
        else:
            result = subprocess.run(
                ['cmake-format', '-i', file_path],
                capture_output=True,
                text=True
            )
            if result.returncode == 0:
                print(f"Formatted {file_path}")
                return True
            else:
                print(f"Failed to format {file_path}")
                return False
    except subprocess.CalledProcessError as e:
        print(f"Error formatting {file_path}: {e}")
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: cmake_formatter.py [--check|--fix] [--compile-commands <path>]")
        sys.exit(1)

    mode = sys.argv[1]
    check_only = mode == "--check"
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    if not check_cmake_format_installed():
        print("Error: cmake-format is not installed. Please install it first:")
        print("pip install cmake-format")
        sys.exit(1)

    # Find all CMake files
    cmake_files = find_cmake_files(root_dir)

    if not cmake_files:
        print("No CMake files found")
        sys.exit(0)

    # Process each file
    status = 0
    for file_path in cmake_files:
        if not format_file(file_path, check_only):
            status = 1

    sys.exit(status)

if __name__ == "__main__":
    main() 