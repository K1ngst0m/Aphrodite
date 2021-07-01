<br>

![logo](Resources/aph-logo.png)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/c%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)

# Aphrodite Game Engine

A game engine written in C++17, work in progress, developing on gentoo.

The Aphrodite Game Engine is licensed under the MIT license.

Until now this engine has been developed for the GNU/Linux OS only.

***

## Features

* **Project**
  * use shell && python scripts for project generating
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
    
***

## Build

```shell
$ git clone https://github.com/npchitman/Aphrodite
$ cd Aphrodite
$ mkdir build && cd build
$ cmake ../ && make
```

## Technical Information

<ins>*Supported IDEs:*</ins>
* Clion 2021
* Visual Studio 2019
* neovim with my personal [profile](https://github.com/npchitman/vimq)

<ins>*Languages:*</ins>
* C++17 (Core)
* GLSL (Shader Language)
* Python (Project Setup)

<ins>*External Libraries Used:*</ins>
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

***