#!/usr/bin/env python3

import re
import sys
import pathlib
from typing import List, Tuple, Set
import json

class IncludeSorter:
    def __init__(self):
        # Define include categories and their priorities
        self.categories = {
            'current_header': (0, r'^#include "([^/]+).h"$'),  # Same component header
            'project_core': (1, r'^#include "common/|^#include "math/'),  # Core engine components
            'project_headers': (2, r'^#include "([^"]+)"'),    # Other project headers
            'third_party': (3, r'^#include "(glm/|stb/|tiny_gltf|slang|SDL3/|imgui)'), # Third-party libs
            'system_headers': (4, r'^#include <[^>]+>$'),      # System/STL headers
        }

    def categorize_include(self, line: str) -> Tuple[int, str]:
        """Categorize an include line and return (priority, line)"""
        for category, (priority, pattern) in self.categories.items():
            if re.match(pattern, line):
                return priority, line
        return 999, line  # Unknown category goes last

    def sort_includes(self, filename: str) -> List[str]:
        with open(filename, 'r') as f:
            lines = f.readlines()

        # Find include block
        include_start = -1
        include_end = -1
        includes = []
        
        for i, line in enumerate(lines):
            if line.strip().startswith('#include'):
                if include_start == -1:
                    include_start = i
                include_end = i
                includes.append(line.strip())
            elif include_start != -1 and line.strip() and include_end != -1:
                break

        if include_start == -1:
            return lines

        # Sort includes within their categories
        sorted_includes = []
        current_category = -1
        categorized = [(self.categorize_include(inc), inc) for inc in includes]
        categorized.sort()

        # Add newlines between categories
        for (priority, _), inc in categorized:
            if current_category != -1 and priority != current_category:
                sorted_includes.append('')
            sorted_includes.append(inc)
            current_category = priority

        # Replace the original includes with sorted ones
        result = (
            lines[:include_start] +
            [f"{line}\n" for line in sorted_includes] +
            lines[include_end + 1:]
        )

        return result

    def check_file(self, filename: str) -> bool:
        """Check if file needs reordering. Returns True if file is already properly ordered."""
        original = open(filename).readlines()
        sorted_lines = self.sort_includes(filename)
        return original == sorted_lines

    def fix_file(self, filename: str) -> bool:
        """Fix include ordering in file. Returns True if file was modified."""
        if not self.check_file(filename):
            sorted_lines = self.sort_includes(filename)
            with open(filename, 'w') as f:
                f.writelines(sorted_lines)
            return True
        return False

def get_source_files(compile_commands_path: str) -> Set[str]:
    """Extract source files from compile_commands.json"""
    with open(compile_commands_path) as f:
        commands = json.load(f)
    
    source_files = set()
    for cmd in commands:
        if cmd['file'].endswith(('.cpp', '.h', '.hpp')):
            source_files.add(cmd['file'])
    return source_files

def main():
    if len(sys.argv) < 2:
        print("Usage: include_sorter.py [--check|--fix] [--compile-commands <path>] <files...>")
        sys.exit(1)

    mode = sys.argv[1]
    files = []
    compile_commands = None

    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == '--compile-commands' and i + 1 < len(sys.argv):
            compile_commands = sys.argv[i + 1]
            i += 2
        else:
            files.append(sys.argv[i])
            i += 1

    if compile_commands:
        files.extend(get_source_files(compile_commands))

    if not files:
        print("No files specified")
        sys.exit(1)

    sorter = IncludeSorter()
    
    if mode == "--check":
        status = 0
        for file in files:
            if not sorter.check_file(file):
                print(f"❌ {file} needs include reordering")
                status = 1
            else:
                print(f"✅ {file} includes are properly ordered")
        sys.exit(status)
    
    elif mode == "--fix":
        for file in files:
            if sorter.fix_file(file):
                print(f"Fixed includes in {file}")
            else:
                print(f"No changes needed in {file}")

if __name__ == "__main__":
    main() 