project(Hazel)

add_subdirectory(3rdparty/glad)
add_subdirectory(3rdparty/spdlog-1.8.5)
add_subdirectory(3rdparty/glfw)
add_subdirectory(3rdparty/imgui)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(3rdparty/glad/include)
include_directories(3rdparty/imgui)
include_directories(3rdparty/glm)
include_directories(3rdparty/stb)
include_directories(3rdparty/entt/include)

add_library(${PROJECT_NAME} STATIC
        # header
        src/Hazel.h src/Hazel/Core/Base.h src/Hazel/Core/EntryPoint.h

        # application
        src/Hazel/Core/Application.cpp src/Hazel/Core/Application.h

        # log
        src/Hazel/Core/Log.cpp src/Hazel/Core/Log.h

        # events
        src/Hazel/Events/Event.h src/Hazel/Events/ApplicationEvent.h src/Hazel/Events/KeyEvent.h src/Hazel/Events/MouseEvent.h

        # precompile headers
        src/hzpch.cpp src/hzpch.h

        # window
        src/Hazel/Core/Window.h src/Platform/Linux/LinuxWindow.cpp src/Platform/Linux/LinuxWindow.h

        # layer
        src/Hazel/Core/Layer.cpp src/Hazel/Core/Layer.h src/Hazel/Core/LayerStack.cpp src/Hazel/Core/LayerStack.h
        src/Hazel/ImGui/ImGuiLayer.cpp src/Hazel/ImGui/ImGuiLayer.h

        # input
        src/Hazel/Core/Input.h src/Platform/Linux/LinuxInput.cpp src/Hazel/Core/KeyCodes.h src/Hazel/Core/MouseCodes.h

        # imgui
        src/Hazel/ImGui/ImGuiBuild.cpp src/Hazel/Renderer/GraphicsContext.cpp src/Hazel/Renderer/GraphicsContext.h src/Platform/OpenGL/OpenGLContext.cpp src/Platform/OpenGL/OpenGLContext.h src/Hazel/Renderer/Shader.cpp src/Hazel/Renderer/Shader.h src/Hazel/Renderer/Buffer.cpp src/Hazel/Renderer/Buffer.h src/Hazel/Renderer/Renderer.cpp src/Hazel/Renderer/Renderer.h src/Platform/OpenGL/OpenGLBuffer.cpp src/Platform/OpenGL/OpenGLBuffer.h src/Hazel/Renderer/VertexArray.cpp src/Hazel/Renderer/VertexArray.h src/Platform/OpenGL/OpenGLVertexArray.cpp src/Platform/OpenGL/OpenGLVertexArray.h src/Hazel/Renderer/RendererAPI.h src/Hazel/Renderer/RenderCommand.cpp src/Hazel/Renderer/RenderCommand.h src/Hazel/Renderer/RendererAPI.cpp src/Platform/OpenGL/OpenGLRendererAPI.cpp src/Platform/OpenGL/OpenGLRendererAPI.h src/Hazel/Renderer/OrthographicCamera.cpp src/Hazel/Renderer/OrthographicCamera.h src/Platform/OpenGL/OpenGLShader.cpp src/Platform/OpenGL/OpenGLShader.h src/Hazel/Core/TimeStep.h src/Hazel/Renderer/Texture.cpp src/Hazel/Renderer/Texture.h src/Platform/OpenGL/OpenGLTexture.cpp src/Platform/OpenGL/OpenGLTexture.h src/Hazel/Renderer/OrthographicCameraController.cpp src/Hazel/Renderer/OrthographicCameraController.h src/Hazel/Renderer/Renderer2D.cpp src/Hazel/Renderer/Renderer2D.h src/Hazel/Debug/Instrumentor.h src/Hazel/Core/Window.cpp src/Hazel/Renderer/Framebuffer.cpp src/Hazel/Renderer/Framebuffer.h src/Platform/OpenGL/OpenGLFramebuffer.cpp src/Platform/OpenGL/OpenGLFramebuffer.h src/Hazel/Scene/Scene.cpp src/Hazel/Scene/Scene.h src/Hazel/Scene/Components.h 3rdparty/entt/include/entt.hpp src/Hazel/Scene/Entity.cpp src/Hazel/Scene/Entity.h)

add_definitions(-DSTB_IMAGE_IMPLEMENTATION)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/hzpch.h)

target_link_libraries(${PROJECT_NAME} PUBLIC glad glfw GL imgui spdlog::spdlog)
