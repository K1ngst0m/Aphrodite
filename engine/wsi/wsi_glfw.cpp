#include "wsi.h"
#include "app/input/eventManager.h"
#include "api/vulkan/instance.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "imgui_impl_glfw.h"

using namespace aph;

static Key glfwKeyCast(int key)
{
#define k(glfw, aph) \
    case GLFW_KEY_##glfw: \
        return Key::aph
    switch(key)
    {
        k(A, A);
        k(B, B);
        k(C, C);
        k(D, D);
        k(E, E);
        k(F, F);
        k(G, G);
        k(H, H);
        k(I, I);
        k(J, J);
        k(K, K);
        k(L, L);
        k(M, M);
        k(N, N);
        k(O, O);
        k(P, P);
        k(Q, Q);
        k(R, R);
        k(S, S);
        k(T, T);
        k(U, U);
        k(V, V);
        k(W, W);
        k(X, X);
        k(Y, Y);
        k(Z, Z);
        k(LEFT_CONTROL, LeftCtrl);
        k(LEFT_ALT, LeftAlt);
        k(LEFT_SHIFT, LeftShift);
        k(ENTER, Return);
        k(SPACE, Space);
        k(ESCAPE, Escape);
        k(LEFT, Left);
        k(RIGHT, Right);
        k(UP, Up);
        k(DOWN, Down);
        k(0, _0);
        k(1, _1);
        k(2, _2);
        k(3, _3);
        k(4, _4);
        k(5, _5);
        k(6, _6);
        k(7, _7);
        k(8, _8);
        k(9, _9);
    default:
        return Key::Unknown;
    }
#undef k
}

static void cursorCB(GLFWwindow* window, double x, double y)
{
    auto* wsi = static_cast<WSI*>(glfwGetWindowUserPointer(window));

    static double lastX = wsi->getWidth() / 2;
    static double lastY = wsi->getHeight() / 2;

    double deltaX = lastX - x;
    double deltaY = lastY - y;
    lastX         = x;
    lastY         = y;

    EventManager::GetInstance().pushEvent(MouseMoveEvent{deltaX, deltaY, x, y});
}

static void keyCB(GLFWwindow* window, int key, int _, int action, int mods)
{
    KeyState state{};
    switch(action)
    {
    case GLFW_PRESS:
        state = KeyState::Pressed;
        break;
    case GLFW_RELEASE:
        state = KeyState::Released;
        break;
    case GLFW_REPEAT:
        state = KeyState::Repeat;
        break;
    }

    auto  gkey = glfwKeyCast(key);
    auto* wsi  = static_cast<WSI*>(glfwGetWindowUserPointer(window));

    if(action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
    {
        wsi->close();
    }
    else if(action == GLFW_PRESS && key == GLFW_KEY_1)
    {
        static bool visible = false;
        glfwSetInputMode(window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        visible = !visible;
    }
    else
    {
        EventManager::GetInstance().pushEvent(KeyboardEvent{gkey, state});
    }
}

static void buttonCB(GLFWwindow* window, int button, int action, int _)
{
    MouseButton btn;
    switch(button)
    {
    default:
    case GLFW_MOUSE_BUTTON_LEFT:
        btn = MouseButton::Left;
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        btn = MouseButton::Right;
        break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
        btn = MouseButton::Middle;
        break;
    }

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    EventManager::GetInstance().pushEvent(MouseButtonEvent{btn, x, y, action == GLFW_PRESS});
}

static void windowResizeCallback(GLFWwindow* window, int width, int height)
{
    auto* wsi = static_cast<WSI*>(glfwGetWindowUserPointer(window));
    wsi->resize(width, height);

    WindowResizeEvent resizeEvent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    // Push the event to your event queue or handle it immediately
    EventManager::GetInstance().pushEvent(resizeEvent);
}

void WSI::init()
{
    assert(glfwInit());
    assert(glfwVulkanSupported());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window    = (void*)glfwCreateWindow(m_width, m_height, "Aphrodite Engine", nullptr, nullptr);
    auto window = static_cast<GLFWwindow*>(m_window);
    assert(window);

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCB);
    glfwSetCursorPosCallback(window, cursorCB);
    glfwSetMouseButtonCallback(window, buttonCB);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);
}

VkSurfaceKHR WSI::getSurface(vk::Instance* instance)
{
    VkSurfaceKHR surface{};
    glfwCreateWindowSurface(instance->getHandle(), (GLFWwindow*)m_window, nullptr, &surface);
    return surface;
};

WSI::~WSI()
{
    glfwDestroyWindow((GLFWwindow*)m_window);
    glfwTerminate();
}

bool WSI::update()
{
    if(glfwWindowShouldClose((GLFWwindow*)m_window))
        return false;

    glfwPollEvents();

    EventManager::GetInstance().processAllAsync();

    if (m_enabledUI)
    {
        ImGui_ImplGlfw_NewFrame();
    }

    EventManager::GetInstance().flush();
    return true;
};

void WSI::close()
{
    glfwSetWindowShouldClose((GLFWwindow*)m_window, true);
};

void WSI::resize(uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    int w, h;
    glfwGetFramebufferSize((GLFWwindow*)m_window, &w, &h);

    if (w != m_width || h != m_height)
    {
        glfwSetWindowSize((GLFWwindow*)m_window, m_width, m_height);
    }
}

std::vector<const char*> aph::WSI::getRequiredExtensions()
{
    std::vector<const char*> extensions{};
    {
        uint32_t     glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        extensions     = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
    }
    return extensions;
}

bool WSI::initUI()
{
    if (m_enabledUI)
    {
        return ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)m_window, true);
    }
    return false;
};

void aph::WSI::deInitUI()
{
    if (m_enabledUI)
    {
        ImGui_ImplGlfw_Shutdown();
    }
}
