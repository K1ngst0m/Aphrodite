project(Aphrodite-Runtime)

link_directories(${CMAKE_BINARY_DIR}/Libs)

add_library(${PROJECT_NAME} STATIC
        # header
        src/Aphrodite.h src/Aphrodite/Core/Base.h src/Aphrodite/Core/EntryPoint.h

        # application
        src/Aphrodite/Core/Application.cpp src/Aphrodite/Core/Application.h

        # log
        src/Aphrodite/Core/Log.cpp src/Aphrodite/Core/Log.h

        # events
        src/Aphrodite/Events/Event.h src/Aphrodite/Events/ApplicationEvent.h src/Aphrodite/Events/KeyEvent.h src/Aphrodite/Events/MouseEvent.h

        # precompile headers
        src/pch.cpp src/pch.h

        # window
        src/Aphrodite/Core/Window.h src/Platform/Linux/LinuxWindow.cpp src/Platform/Linux/LinuxWindow.h

        # layer
        src/Aphrodite/Core/Layer.cpp src/Aphrodite/Core/Layer.h src/Aphrodite/Core/LayerStack.cpp src/Aphrodite/Core/LayerStack.h
        src/Aphrodite/ImGui/ImGuiLayer.cpp src/Aphrodite/ImGui/ImGuiLayer.h

        # input
        src/Aphrodite/Core/Input.h src/Platform/Linux/LinuxInput.cpp src/Aphrodite/Core/KeyCodes.h src/Aphrodite/Core/MouseCodes.h

        # imgui
        src/Aphrodite/ImGui/ImGuiBuild.cpp src/Aphrodite/Renderer/GraphicsContext.cpp src/Aphrodite/Renderer/GraphicsContext.h src/Platform/OpenGL/OpenGLContext.cpp src/Platform/OpenGL/OpenGLContext.h src/Aphrodite/Renderer/Shader.cpp src/Aphrodite/Renderer/Shader.h src/Aphrodite/Renderer/Buffer.cpp src/Aphrodite/Renderer/Buffer.h src/Aphrodite/Renderer/Renderer.cpp src/Aphrodite/Renderer/Renderer.h src/Platform/OpenGL/OpenGLBuffer.cpp src/Platform/OpenGL/OpenGLBuffer.h src/Aphrodite/Renderer/VertexArray.cpp src/Aphrodite/Renderer/VertexArray.h src/Platform/OpenGL/OpenGLVertexArray.cpp src/Platform/OpenGL/OpenGLVertexArray.h src/Aphrodite/Renderer/RendererAPI.h src/Aphrodite/Renderer/RenderCommand.cpp src/Aphrodite/Renderer/RenderCommand.h src/Aphrodite/Renderer/RendererAPI.cpp src/Platform/OpenGL/OpenGLRendererAPI.cpp src/Platform/OpenGL/OpenGLRendererAPI.h src/Aphrodite/Renderer/OrthographicCamera.cpp src/Aphrodite/Renderer/OrthographicCamera.h src/Platform/OpenGL/OpenGLShader.cpp src/Platform/OpenGL/OpenGLShader.h src/Aphrodite/Core/TimeStep.h src/Aphrodite/Renderer/Texture.cpp src/Aphrodite/Renderer/Texture.h src/Platform/OpenGL/OpenGLTexture.cpp src/Platform/OpenGL/OpenGLTexture.h src/Aphrodite/Renderer/OrthographicCameraController.cpp src/Aphrodite/Renderer/OrthographicCameraController.h src/Aphrodite/Renderer/Renderer2D.cpp src/Aphrodite/Renderer/Renderer2D.h src/Aphrodite/Debug/Instrumentor.h src/Aphrodite/Core/Window.cpp src/Aphrodite/Renderer/Framebuffer.cpp src/Aphrodite/Renderer/Framebuffer.h src/Platform/OpenGL/OpenGLFramebuffer.cpp src/Platform/OpenGL/OpenGLFramebuffer.h

        # Scene
        src/Aphrodite/Scene/Scene.cpp src/Aphrodite/Scene/Scene.h src/Aphrodite/Scene/Components.h
        src/Aphrodite/Scene/Entity.cpp src/Aphrodite/Scene/Entity.h src/Aphrodite/Renderer/Camera.h src/Aphrodite/Scene/SceneCamera.cpp src/Aphrodite/Scene/SceneCamera.h src/Aphrodite/Scene/ScriptableEntity.cpp src/Aphrodite/Scene/ScriptableEntity.h src/Aphrodite/Core/PlatformDetection.h src/Aphrodite/Core/Assert.h src/Aphrodite/Scene/SceneSerializer.cpp src/Aphrodite/Scene/SceneSerializer.h

        # platform Util
        src/Aphrodite/Utils/PlatformUtils.h src/Platform/Linux/LinuxPlatformUtils.cpp

        # math
        src/Aphrodite/Math/Math.cpp src/Aphrodite/Math/Math.h src/Aphrodite/Renderer/EditorCamera.cpp src/Aphrodite/Renderer/EditorCamera.h src/Aphrodite/Core/Timer.h src/Aphrodite/Renderer/UniformBuffer.cpp src/Aphrodite/Renderer/UniformBuffer.h src/Platform/OpenGL/OpenGLUniformBuffer.cpp src/Platform/OpenGL/OpenGLUniformBuffer.h src/Aphrodite/Fonts/IconsFontAwesome5Pro.h
        src/Aphrodite/ImGui/Utilities/ImGuiAssetBrowser.cpp src/Aphrodite/ImGui/Utilities/ImGuiAssetBrowser.h src/Aphrodite/ImGui/Utilities/ImGuiConsole.cpp src/Aphrodite/ImGui/Utilities/ImGuiConsole.h)

add_definitions(-DSTB_IMAGE_IMPLEMENTATION -DAPH_ENABLE_ASSERTS -DAPH_DEBUG)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

set(opengl_lib glad glfw GL)
set(vulkan shaderc_shared spirv-cross-core spirv-cross-glsl SPIRV-Tools)
set(ui_lib imgui imGuizmo)
set(other_lib spdlog::spdlog yaml-cpp entt)

target_link_libraries(${PROJECT_NAME} PUBLIC
        ${vulkan}
        ${opengl_lib}
        ${ui_lib}
        ${other_lib}
        )