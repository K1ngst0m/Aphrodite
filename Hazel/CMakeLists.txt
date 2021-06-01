project(Hazel)

add_subdirectory(3rdparty/glad)
add_subdirectory(3rdparty/spdlog-1.8.5)
add_subdirectory(3rdparty/glfw)
add_subdirectory(3rdparty/imgui)


include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(3rdparty/glad/include)
include_directories(3rdparty/imgui)
include_directories(3rdparty/glm)

add_library(${PROJECT_NAME} STATIC
        # header
        src/Hazel.h src/Hazel/Core.h src/Hazel/EntryPoint.h

        # application
        src/Hazel/Application.cpp src/Hazel/Application.h

        # log
        src/Hazel/Log.cpp src/Hazel/Log.h

        # events
        src/Hazel/Events/Event.h src/Hazel/Events/ApplicationEvent.h src/Hazel/Events/KeyEvent.h src/Hazel/Events/MouseEvent.h

        # precompile headers
        src/hzpch.cpp src/hzpch.h

        # window
        src/Hazel/Window.h src/Platform/Linux/LinuxWindow.cpp src/Platform/Linux/LinuxWindow.h

        # layer
        src/Hazel/Layer.cpp src/Hazel/Layer.h src/Hazel/LayerStack.cpp src/Hazel/LayerStack.h
        src/Hazel/ImGui/ImGuiLayer.cpp src/Hazel/ImGui/ImGuiLayer.h

        # imgui
        src/Hazel/Input.h src/Platform/Linux/LinuxInput.cpp src/Platform/Linux/LinuxInput.h src/Hazel/KeyCodes.h src/Hazel/MouseButtonCodes.h src/Hazel/ImGui/ImGuiBuild.cpp src/Hazel/Renderer/GraphicsContext.h src/Platform/OpenGL/OpenGLContext.cpp src/Platform/OpenGL/OpenGLContext.h src/Hazel/Renderer/Shader.cpp src/Hazel/Renderer/Shader.h src/Hazel/Renderer/Buffer.cpp src/Hazel/Renderer/Buffer.h src/Hazel/Renderer/Renderer.cpp src/Hazel/Renderer/Renderer.h src/Platform/OpenGL/OpenGLBuffer.cpp src/Platform/OpenGL/OpenGLBuffer.h src/Hazel/Renderer/VertexArray.cpp src/Hazel/Renderer/VertexArray.h src/Platform/OpenGL/OpenGLVertexArray.cpp src/Platform/OpenGL/OpenGLVertexArray.h src/Hazel/Renderer/RenderAPI.cpp src/Hazel/Renderer/RenderAPI.h)


target_precompile_headers(${PROJECT_NAME} PRIVATE src/hzpch.h)

target_link_libraries(${PROJECT_NAME} PUBLIC glad glfw GL imgui spdlog::spdlog)
