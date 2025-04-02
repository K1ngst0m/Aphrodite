#!/usr/bin/env python3
import os
import re
import sys
import argparse
from pathlib import Path
import subprocess
from collections import defaultdict

def parse_cmake_file(cmake_file, is_main_cmake=False):
    """Parse CMakeLists.txt file to extract target name and dependencies."""
    target_name = None
    dependencies = []
    
    with open(cmake_file, 'r') as f:
        content = f.read()
        
        if is_main_cmake:
            # For the main aphrodite target (src/CMakeLists.txt)
            target_name = "aphrodite::all"
            
            # Find the dependencies of the main target
            main_link_match = re.search(r'target_link_libraries\s*\(\s*aphrodite\s*\n([^)]+)\)', content, re.DOTALL)
            if main_link_match:
                link_block = main_link_match.group(1).strip()
                
                current_visibility = None
                for line in link_block.split('\n'):
                    line = line.strip()
                    if not line or line.startswith('#'):
                        continue
                    
                    # Check for visibility specifiers
                    if line == 'PRIVATE':
                        current_visibility = 'PRIVATE'
                        continue
                    elif line == 'PUBLIC':
                        current_visibility = 'PUBLIC'
                        continue
                    elif line == 'INTERFACE':
                        current_visibility = 'INTERFACE'
                        continue
                    
                    # Check if the line contains a dependency
                    if line.startswith('aphrodite::'):
                        dependencies.append((line, current_visibility or 'PUBLIC'))  # Default to PUBLIC for main target
            
            return target_name, dependencies
        
        # Extract target name
        target_match = re.search(r'aph_setup_target\s*\(\s*(\w+)', content)
        if target_match:
            module_suffix = target_match.group(1)
            internal_name = f"aph-{module_suffix}"
            alias_name = f"aphrodite::{module_suffix}"
            target_name = alias_name
        
        # Extract dependencies - use more flexible pattern to match target_link_libraries block
        link_pattern = re.escape(internal_name) + r'[^)]+\)'
        link_sections = re.findall(r'target_link_libraries\s*\(\s*(' + link_pattern + r')', content, re.DOTALL)
        
        if link_sections and target_name:
            for link_section in link_sections:
                # Remove the target name from the beginning
                section_without_target = re.sub(r'^\s*' + re.escape(internal_name), '', link_section)
                
                # Process each line for dependencies
                current_visibility = None
                for line in section_without_target.split('\n'):
                    line = line.strip()
                    # Skip empty lines, comments, and the closing parenthesis
                    if not line or line.startswith('#') or line == ')':
                        continue
                    
                    # Check for visibility specifiers
                    if line == 'PRIVATE':
                        current_visibility = 'PRIVATE'
                        continue
                    elif line == 'PUBLIC':
                        current_visibility = 'PUBLIC'
                        continue
                    elif line == 'INTERFACE':
                        current_visibility = 'INTERFACE'
                        continue
                    
                    # Check for inline visibility specifier followed by dependencies
                    if line.startswith('PRIVATE '):
                        current_visibility = 'PRIVATE'
                        line = line[len('PRIVATE '):].strip()
                    elif line.startswith('PUBLIC '):
                        current_visibility = 'PUBLIC'
                        line = line[len('PUBLIC '):].strip()
                    elif line.startswith('INTERFACE '):
                        current_visibility = 'INTERFACE'
                        line = line[len('INTERFACE '):].strip()
                    
                    # Find all dependencies in this line
                    # Split by whitespace and handle potential commas
                    parts = re.split(r'[ \t,]+', line)
                    for part in parts:
                        part = part.strip()
                        if not part:
                            continue
                            
                        # Only include our own dependencies
                        if part.startswith('aph-') or part.startswith('aphrodite::'):
                            # Convert aph-xxx to aphrodite::xxx format
                            if part.startswith('aph-'):
                                part = f"aphrodite::{part[4:]}"  # Convert to aphrodite::xxx
                            
                            # Store dependency with its visibility
                            # Default to PRIVATE if no visibility is specified
                            dependencies.append((part, current_visibility or 'PRIVATE'))
    
    return target_name, dependencies

def find_redundant_dependencies(modules):
    """Find redundant public dependencies.
    
    If target S has a PUBLIC link to A and B, and A has a PUBLIC link to B,
    then S's direct dependency on B is redundant.
    """
    redundant_deps = []
    
    # Build a graph of public dependencies
    public_deps = {}
    for module, deps in modules.items():
        public_deps[module] = set()
        for dep, visibility in deps:
            if visibility == 'PUBLIC':
                public_deps[module].add(dep)
    
    # For each module
    for module, deps in modules.items():
        direct_deps = set(dep for dep, _ in deps)
        
        # Check each direct dependency
        for dep, visibility in deps:
            if visibility != 'PUBLIC':
                continue
                
            # Get the public dependencies of this dependency
            if dep in public_deps:
                for transitive_dep in public_deps[dep]:
                    # If the transitive dependency is also a direct dependency
                    if transitive_dep in direct_deps and (module, dep, transitive_dep) not in redundant_deps:
                        redundant_deps.append((module, dep, transitive_dep))
    
    return redundant_deps


font_list = "Helvetica,Arial,sans-serif"
bgcolor = "transparent"

node_color_1 = "#f1efec"
node_color_2 = "#d4c9be"
node_font_color = "#222831"

border_color = "#8A8A8A"

edge_color_1 = "#ff6500"
edge_color_2 = "#1e3e62"
edge_color_3 = "#e9363c"
edge_color_4 = "#1f7d53"

def generate_dot_graph(modules, include_visibility=True, redundant_deps=None):
    """Generate DOT graph from module dependencies."""
    dot = ['digraph CMakeDependencyGraph {',
           f'  graph [rankdir=LR, fontname="{font_list}", nodesep=0.3, ranksep=0.8, splines=true, overlap=false, bgcolor="{bgcolor}"];',
           f'  node [shape=Mrecord, style="filled, bold", fontname="{font_list}", fontsize=15, fontcolor="{node_font_color}", penwidth=1.5];',
           f'  edge [fontname="{font_list}", fontsize=8, arrowhead=vee];',
           f"""
           subgraph cluster_legend {{
               style=filled;
               fillcolor="{bgcolor}";
               // Set the border color to match the background and remove its width
               color="{bgcolor}";
               penwidth=0;

               key [shape=plaintext, margin=0
               label=<
                    <TABLE BORDER="0" BGCOLOR="{bgcolor}" CELLBORDER="1" CELLSPACING="0" CELLPADDING="4">
                        <TR>
                            <TD><FONT COLOR="{edge_color_1}">➔</FONT></TD>
                            <TD>Public</TD>
                        </TR>
                        <TR>
                            <TD><FONT COLOR="{edge_color_2}">➔</FONT></TD>
                            <TD>Private</TD>
                        </TR>
                        <TR>
                            <TD><FONT COLOR="{edge_color_4}">➔</FONT></TD>
                            <TD>Interface</TD>
                        </TR>
                    </TABLE>
               >];
           }}
           """
           '']
    
    # Add nodes - ensure all modules are included even if they have no dependencies
    for module in modules.keys():
        # Special handling for the main target
        if module == "aphrodite::all":
            dot.append(f'  "{module}" [fillcolor="{node_color_1}", style="filled,bold", peripheries=2];')
        else:
            dot.append(f'  "{module}" [fillcolor="{node_color_2}"];')
    
    # Add edges
    for module, deps in modules.items():
        for dep, visibility in deps:
            if include_visibility and visibility:
                edge_attr = ''
                # Check if this is a redundant dependency
                is_redundant = redundant_deps and any(r for r in redundant_deps if r[0] == module and r[2] == dep)
                
                if is_redundant:
                    edge_attr = f' [color="{edge_color_3}"]'
                elif visibility == 'PUBLIC':
                    edge_attr = f' [color="{edge_color_1}"]'
                elif visibility == 'PRIVATE':
                    edge_attr = f' [color="{edge_color_2}"]'
                elif visibility == 'INTERFACE':
                    edge_attr = f' [color="{edge_color_4}"]'
                
                dot.append(f'  "{module}" -> "{dep}"{edge_attr};')
            else:
                dot.append(f'  "{module}" -> "{dep}";')
    
    dot.append('}')
    return '\n'.join(dot)

def main():
    parser = argparse.ArgumentParser(description='Generate CMake dependency graph from CMakeLists.txt files')
    parser.add_argument('source_dir', nargs='?', default='.', help='Source directory containing CMakeLists.txt files')
    parser.add_argument('-o', '--output', default='cmake_dependency_graph.dot', help='Output DOT file')
    parser.add_argument('-i', '--image', help='Output image file (requires graphviz)')
    parser.add_argument('-f', '--format', default='png', choices=['png', 'svg', 'pdf'], help='Output image format')
    parser.add_argument('--no-visibility', action='store_true', help='Don\'t show visibility (PUBLIC/PRIVATE) in the graph')
    parser.add_argument('--no-redundancy-check', action='store_true', help='Disable redundant dependency checking')
    parser.add_argument('-v', '--verbose', action='store_true', help='Enable verbose output')
    
    args = parser.parse_args()
    
    source_dir = os.path.abspath(args.source_dir)
    modules = {}
    src_cmakelist = os.path.join(source_dir, 'CMakeLists.txt')
    
    # Add the main aphrodite target first if it exists
    if os.path.exists(src_cmakelist):
        target_name, dependencies = parse_cmake_file(src_cmakelist, is_main_cmake=True)
        if target_name and dependencies:
            modules[target_name] = dependencies
            if args.verbose:
                print(f"Found main target: {target_name} with {len(dependencies)} dependencies")
                for dep, vis in dependencies:
                    print(f"  {vis if vis else 'UNKNOWN'}: {dep}")
    
    # Walk through all subdirectories
    for root, _, files in os.walk(source_dir):
        if 'CMakeLists.txt' in files and root != source_dir:
            cmake_path = os.path.join(root, 'CMakeLists.txt')
            target_name, dependencies = parse_cmake_file(cmake_path)
            
            if target_name:
                if args.verbose:
                    print(f"Found module: {target_name} with {len(dependencies)} dependencies")
                    for dep, vis in dependencies:
                        print(f"  {vis if vis else 'UNKNOWN'}: {dep}")
                        
                modules[target_name] = dependencies
    
    # Check for redundant dependencies
    redundant_deps = None
    if not args.no_redundancy_check:
        redundant_deps = find_redundant_dependencies(modules)
        if redundant_deps and (args.verbose or True):  # Always show redundant deps
            print("\nRedundant dependencies found:")
            for module, through, dep in redundant_deps:
                print(f"  {module} -> {dep} is redundant (already accessible through {through})")
    
    # Generate DOT graph
    dot_graph = generate_dot_graph(modules, not args.no_visibility, redundant_deps)
    
    # Write to DOT file
    with open(args.output, 'w') as f:
        f.write(dot_graph)
    
    print(f"CMake dependency graph written to {args.output}")
    print(f"Found {len(modules)} modules with {sum(len(deps) for deps in modules.values())} dependencies")
    
    # Generate image if requested
    if args.image:
        try:
            cmd = ['dot', f'-T{args.format}', args.output, '-o', args.image]
            subprocess.run(cmd, check=True)
            print(f"Image generated: {args.image}")
        except subprocess.CalledProcessError:
            print("Error: Failed to generate image. Is Graphviz installed?")
        except FileNotFoundError:
            print("Error: Graphviz 'dot' command not found. Please install Graphviz.")

if __name__ == '__main__':
    main()
