<br>

![logo](Resources/aph-logo.png)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/c%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)

# Aphrodite Game Engine

A game engine written in C++17, work in progress, developing on Gentoo.

The Aphrodite Game Engine is licensed under the MIT license.

Until now this engine has been developed for the GNU/Linux OS only.

***

## Features

* **Project**
  * Written in modern C++
  * Use shell && Python scripts for project generation
  * Cmake Setup
    
* **Core**
  * Console Logging
  * Event System and Events
  * LayerStack and Layers
  * Internal Key, Mouse Codes and Input polling
  * Timesteps, Vsync, Delta Time
  * Shaders and Textures
  * Entity Component System
    
* **Rendering**
  * Modern OpenGL setup, using Glad
  * 2D Rendering pipeline
  * Simplified API for drawing colored and textured quads
    
* **Editor**
  * ImGui, Docking, Frambuffers and ImGui Viewport
  * Editor Gizmos
  * Editor Camera
  * Native C++ scripting
  * ImGui Console Logging
  * ImGui Scene Hierarchy Panel
  * ImGui Properties Panel
  * Scene Serialization and Deserialization using YAML

Editor UI in [Bspwm](https://wiki.gentoo.org/wiki/Bspwm): 

![editorUI](Resources/screenshot.png)

***

## Installation

Tested on Gentoo Base System, BSPWM

<ins>*Tools required:*</ins>
- CMake 3.17+
- C++17 capable compiler:
  - Clang 5
  - GCC 7
    

<ins>*Supported IDEs:*</ins>
* Clion 2021
* Visual Studio 2019
* Development Workflow: [dotfiles](https://github.com/npchitman/dotfiles)
* Neovim with my personal [profile](https://github.com/npchitman/vimq)

<ins>*Languages:*</ins>
* C++17 (Core)
* GLSL (Shader Language)
* Python (Project Setup)

<ins>*Library dependencies:*</ins>
* [GLFW](https://www.glfw.org/)
* [Glad](https://glad.dav1d.de/)
* [Vulkan](https://www.lunarg.com/vulkan-sdk/)
* [ImGui](https://github.com/ocornut/imgui)
* [spdlog](https://github.com/gabime/spdlog)
* [glm](https://glm.g-truc.net/0.9.9/index.html)
* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h)
* [entt](https://github.com/skypjack/entt)
* [yaml-cpp](https://github.com/jbeder/yaml-cpp)
* [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders)
* [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)
  
<ins>*Set up:*</ins>

- Ensure that all dependencies above are set up correctly
- Clone repository and create build directory
  ```shell
  $ cd Aphrodite
  $ mkdir build && cd build
  ```
- Build with CMake
  - Runtime Lib only
  ```shell
  $ cmake ../ && cmake --build . --target Aphrodite-Runtime #-jx
  ```
  - Runtime Lib && Editor
  ```shell
  $ cmake ../ && cmake --build . -- target Aphrodite-Editor #-jx
  ```

***

## Documentation

[Aphrodite Wiki](https://github.com/npchitman/Aphrodite/wiki) will comming soon!
