#!/usr/bin/env python3

import os
import sys
import subprocess
import json
from typing import List, Set, Optional

def find_source_files(root_dir: str, debug: bool = False) -> Set[str]:
    """Find C++ source and header files in specific directories."""
    source_files = set()
    target_dirs = ["src", "tests", "examples"]
    
    if debug:
        print(f"Looking for source files in project directories: {target_dirs}")
        print(f"Project root: {root_dir}")
    
    for target_dir in target_dirs:
        dir_path = os.path.join(root_dir, target_dir)
        if not os.path.exists(dir_path):
            if debug:
                print(f"Directory not found: {dir_path}")
            continue
        
        if debug:
            print(f"Scanning directory: {dir_path}")
            
        for root, _, files in os.walk(dir_path):
            for file in files:
                if file.endswith(('.cpp', '.h', '.hpp', '.cc', '.cxx', '.c', '.inl')):
                    source_files.add(os.path.join(root, file))
    
    if debug:
        print(f"Found {len(source_files)} files to process")
    
    return source_files

def get_source_files_from_compile_commands(compile_commands_path: str, debug: bool = False) -> Set[str]:
    """Extract source files from compile_commands.json, filtering to only include src, tests, and examples directories."""
    try:
        if debug:
            print(f"Reading compile_commands.json from: {compile_commands_path}")
            
        with open(compile_commands_path) as f:
            commands = json.load(f)
        
        if debug:
            print(f"Found {len(commands)} entries in compile_commands.json")
            
        source_files = set()
        root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        target_dirs = [
            os.path.join(root_dir, "src"),
            os.path.join(root_dir, "tests"),
            os.path.join(root_dir, "examples")
        ]
        
        if debug:
            print(f"Target directories:")
            for d in target_dirs:
                print(f"  - {os.path.abspath(d)}")
        
        for cmd in commands:
            if 'file' in cmd and cmd['file'].endswith(('.cpp', '.h', '.hpp', '.cc', '.cxx', '.c', '.inl')):
                file_path = os.path.abspath(cmd['file'])
                # Only include files that are within the project's specific directories
                if any(file_path.startswith(os.path.abspath(target_dir)) for target_dir in target_dirs):
                    source_files.add(file_path)
                elif debug:
                    print(f"Skipping file outside project directories: {file_path}")
        
        if debug:
            print(f"Found {len(source_files)} project source files to process")
            
        return source_files
    except (json.JSONDecodeError, FileNotFoundError) as e:
        print(f"Error reading compile_commands.json: {e}")
        return set()

def check_clang_format_installed() -> bool:
    """Check if clang-format is installed."""
    try:
        subprocess.run(['clang-format', '--version'], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def format_file(file_path: str, check_only: bool = False) -> bool:
    """Format a single file with clang-format."""
    try:
        if check_only:
            # Create a temporary file with formatted content to compare
            result = subprocess.run(
                ['clang-format', file_path],
                capture_output=True,
                text=True
            )
            
            if result.returncode != 0:
                print(f"Error checking {file_path}: {result.stderr}")
                return False
                
            with open(file_path, 'r') as f:
                original_content = f.read()
                
            if original_content != result.stdout:
                print(f"❌ {file_path} needs formatting")
                return False
            print(f"✅ {file_path} is properly formatted")
            return True
        else:
            result = subprocess.run(
                ['clang-format', '-i', file_path],
                capture_output=True,
                text=True
            )
            if result.returncode == 0:
                print(f"Formatted {file_path}")
                return True
            else:
                print(f"Failed to format {file_path}: {result.stderr}")
                return False
    except subprocess.SubprocessError as e:
        print(f"Error formatting {file_path}: {e}")
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: clang_formatter.py [--check|--fix] [--compile-commands <path>] [--debug] [files...]")
        sys.exit(1)

    mode = sys.argv[1]
    check_only = mode == "--check"
    root_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    
    files = []
    compile_commands = None
    debug = False

    # Parse arguments
    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == '--compile-commands' and i + 1 < len(sys.argv):
            compile_commands = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == '--debug':
            debug = True
            i += 1
        else:
            files.append(sys.argv[i])
            i += 1

    if debug:
        print(f"Running in debug mode")
        print(f"Mode: {'check' if check_only else 'fix'}")
        print(f"Project root: {root_dir}")

    if not check_clang_format_installed():
        print("Error: clang-format is not installed. Please install it first.")
        sys.exit(1)

    # Collect files to process
    if not files:
        if compile_commands:
            files = list(get_source_files_from_compile_commands(compile_commands, debug))
        else:
            files = list(find_source_files(root_dir, debug))

    if not files:
        print("No source files found in src, tests, or examples directories")
        sys.exit(0)

    # Process each file
    status = 0
    for file_path in files:
        if not format_file(file_path, check_only):
            status = 1

    sys.exit(status)

if __name__ == "__main__":
    main() 