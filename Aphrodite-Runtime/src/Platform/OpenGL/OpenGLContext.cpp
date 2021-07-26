//
// Created by npchitman on 6/1/21.
//

#include "OpenGLContext.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "pch.h"

namespace Aph {

    OpenGLContext::OpenGLContext(GLFWwindow *windowHandle)
        : m_WindowHandle(windowHandle){
        APH_CORE_ASSERT(windowHandle, "window handle is null!");
    }

    void OpenGLContext::Init() {
        APH_PROFILE_FUNCTION();

        glfwMakeContextCurrent(m_WindowHandle);

        int status = gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
        APH_CORE_ASSERT(status, "Failed to initialize Glad!");

        m_ContextInfo = ContextInfo(glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));

        APH_CORE_INFO("OpenGL Info >>>");
        APH_CORE_INFO("Vendor: {0}", m_ContextInfo.Vendor);
        APH_CORE_INFO("Renderer: {0}", m_ContextInfo.Renderer);
        APH_CORE_INFO("Version: {0}", m_ContextInfo.Version);

        APH_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5),
                        "Aph-Runtime requires at least OpenGL version 4.5!");
    }

    void OpenGLContext::SwapBuffers() {
        APH_PROFILE_FUNCTION();

        glfwSwapBuffers(m_WindowHandle);
    }

}// namespace Aph-Runtime
