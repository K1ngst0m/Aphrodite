#!/usr/bin/env python3
"""
Container Builder for Aphrodite Project

This script builds the Aphrodite project in a containerized environment
using either Docker or Podman, without affecting the host system.
"""

import os
import sys
import argparse
import subprocess
import shutil
from pathlib import Path


def parse_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Build Aphrodite in a containerized environment."
    )
    
    parser.add_argument(
        "preset",
        nargs="?",
        default="clang-release",
        help="Build preset to use (default: clang-release)",
    )
    
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean previous build results before building",
    )
    
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Enable verbose output",
    )
    
    return parser.parse_args()


def find_container_runtime():
    """Find available container runtime (Docker or Podman)."""
    for runtime in ["docker", "podman"]:
        if shutil.which(runtime):
            print(f"Using {runtime.capitalize()} as container runtime")
            return runtime
    
    print("Error: Neither Docker nor Podman is installed. Please install one of them first.")
    sys.exit(1)


def build_image(runtime, image_name, dockerfile_path, verbose):
    """Build the container image if it doesn't exist."""
    try:
        # Check if image exists
        result = subprocess.run(
            [runtime, "image", "inspect", image_name],
            capture_output=True,
            text=True
        )
        
        if result.returncode != 0:
            print(f"Building container image...")
            cmd = [runtime, "build", "-t", image_name, "-f", dockerfile_path, "."]
            
            if verbose:
                subprocess.run(cmd)
            else:
                subprocess.run(cmd, stdout=subprocess.PIPE)
    except Exception as e:
        print(f"Error building container image: {e}")
        sys.exit(1)


def run_container(runtime, image_name, project_dir, build_preset, clean_build, verbose):
    """Run the container to build the project."""
    try:
        # Create ccache directory if it doesn't exist
        ccache_dir = os.path.join(project_dir, ".ccache")
        os.makedirs(ccache_dir, exist_ok=True)
        
        # Prepare container run command
        cmd = [
            runtime, "run", "--rm",
            "-v", f"{project_dir}:/aphrodite",
            "-v", f"{ccache_dir}:/ccache",
        ]
        
        # Add clean build environment variable if needed
        if clean_build:
            print("Cleaning previous build results...")
            cmd.extend(["--env", "CLEAN_BUILD=1"])
        
        # Add image name and build preset
        cmd.extend([image_name, build_preset])
        
        print(f"Running build with preset: {build_preset}")
        
        if verbose:
            subprocess.run(cmd)
        else:
            subprocess.run(cmd)
        
        print(f"Build completed. Results are in {os.path.join(project_dir, 'build-container')}")
    
    except Exception as e:
        print(f"Error running container: {e}")
        sys.exit(1)


def main():
    """Main function."""
    args = parse_args()
    
    # Get project directory (where the script is run from)
    project_dir = os.getcwd()
    
    # Find container runtime
    runtime = find_container_runtime()
    
    # Container image name
    image_name = "aphrodite-builder"
    
    # Path to Dockerfile
    dockerfile_path = os.path.join(project_dir, "container", "Dockerfile")
    
    # Build the container image if needed
    build_image(runtime, image_name, dockerfile_path, args.verbose)
    
    # Run the container
    run_container(
        runtime=runtime,
        image_name=image_name,
        project_dir=project_dir,
        build_preset=args.preset,
        clean_build=args.clean,
        verbose=args.verbose
    )


if __name__ == "__main__":
    main() 