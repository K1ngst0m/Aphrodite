#include "wsi.h"
#include "imgui_impl_sdl2.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include "event/eventManager.h"
#include "api/vulkan/instance.h"

using namespace aph;

static Key SDL2KeyCast(int key)
{
#define k(sdlk, aph) \
    case SDLK_##sdlk: \
        return Key::aph
    switch(key)
    {
        k(a, A);
        k(b, B);
        k(c, C);
        k(d, D);
        k(e, E);
        k(f, F);
        k(g, G);
        k(h, H);
        k(i, I);
        k(j, J);
        k(k, K);
        k(l, L);
        k(m, M);
        k(n, N);
        k(o, O);
        k(p, P);
        k(q, Q);
        k(r, R);
        k(s, S);
        k(t, T);
        k(u, U);
        k(v, V);
        k(w, W);
        k(x, X);
        k(y, Y);
        k(z, Z);

        k(LCTRL, LeftCtrl);
        k(LALT, LeftAlt);
        k(LSHIFT, LeftShift);
        k(RETURN, Return);
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

void WindowSystem::init()
{
    // Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        CM_LOG_ERR("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        APH_ASSERT(false);
    }

    // Create window
    m_window = (void*)SDL_CreateWindow("Aphrodite Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_width,
                                       m_height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

    if(m_window == nullptr)
    {
        CM_LOG_ERR("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        APH_ASSERT(false);
    }
}

VkSurfaceKHR WindowSystem::getSurface(vk::Instance* instance)
{
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface((SDL_Window*)m_window, instance->getHandle(), &surface);
    return surface;
};

WindowSystem::~WindowSystem()
{
    SDL_DestroyWindow((SDL_Window*)m_window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
}

bool WindowSystem::update()
{
    SDL_Event windowEvent;
    while(SDL_PollEvent(&windowEvent))
    {
        switch(windowEvent.type)
        {
        case SDL_QUIT:
            return false;
        case SDL_KEYDOWN:
        {
            KeyState state{};
            auto     keysym = windowEvent.key.keysym.sym;
            auto     gkey   = SDL2KeyCast(keysym);

            switch(windowEvent.key.type)
            {
            case SDL_KEYDOWN:
            {
                state = KeyState::Pressed;
                if(gkey == Key::Escape)
                {
                    close();
                    return false;
                }

                if(gkey == Key::_1)
                {
                    // TODO cursor visible
                    static bool visible = false;
                    visible             = !visible;
                }
                else
                {
                    EventManager::GetInstance().pushEvent(KeyboardEvent{gkey, state});
                }
            }
            break;
            case SDL_KEYUP:
            {
                state = KeyState::Released;
                EventManager::GetInstance().pushEvent(KeyboardEvent{gkey, state});
            }
            break;
            }

            if(windowEvent.key.repeat)
            {
                state = KeyState::Repeat;
            }
        }
        break;
        case SDL_MOUSEMOTION:
        {
            int x, y;
            SDL_GetMouseState(&x, &y);

            static int lastX = getWidth() / 2;
            static int lastY = getHeight() / 2;

            int deltaX = lastX - x;
            int deltaY = lastY - y;
            lastX      = x;
            lastY      = y;

            EventManager::GetInstance().pushEvent(MouseMoveEvent{deltaX, deltaY, x, y});
        }
        break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        {
            MouseButton btn;
            switch(windowEvent.button.button)
            {
            default:
            case SDL_BUTTON_LEFT:
                btn = MouseButton::Left;
                break;
            case SDL_BUTTON_RIGHT:
                btn = MouseButton::Right;
                break;
            case SDL_BUTTON_MIDDLE:
                btn = MouseButton::Middle;
                break;
            }

            int x, y;
            SDL_GetMouseState(&x, &y);

            EventManager::GetInstance().pushEvent(MouseButtonEvent{btn, x, y, windowEvent.type == SDL_MOUSEBUTTONDOWN});
        }
        break;
        case SDL_WINDOWEVENT_RESIZED:
        {
            resize(windowEvent.window.data1, windowEvent.window.data2);

            WindowResizeEvent resizeEvent{static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height)};

            // Push the event to your event queue or handle it immediately
            EventManager::GetInstance().pushEvent(resizeEvent);
        }
        break;
        default:
            break;
        }
    }

    EventManager::GetInstance().processAllAsync();

    if(m_enabledUI)
    {
        ImGui_ImplSDL2_NewFrame();
    }

    EventManager::GetInstance().flush();
    return true;
};

void WindowSystem::close() {
    // glfwSetWindowShouldClose((GLFWwindow*)m_window, true);
};

void WindowSystem::resize(uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    int  w, h;
    auto window = (SDL_Window*)m_window;
    SDL_GetWindowSize(window, &w, &h);

    if(w != m_width || h != m_height)
    {
        SDL_SetWindowSize(window, m_width, m_height);
    }
}

std::vector<const char*> WindowSystem::getRequiredExtensions()
{
    std::vector<const char*> extensions{};
    {
        uint32_t extensionCount;
        SDL_Vulkan_GetInstanceExtensions((SDL_Window*)m_window, &extensionCount, nullptr);
        extensions.resize(extensionCount);
        SDL_Vulkan_GetInstanceExtensions((SDL_Window*)m_window, &extensionCount, extensions.data());
    }
    return extensions;
}

bool WindowSystem::initUI()
{
    if(m_enabledUI)
    {
        return ImGui_ImplSDL2_InitForVulkan((SDL_Window*)m_window);
    }
    return false;
};

void WindowSystem::deInitUI() const
{
    if(m_enabledUI)
    {
        ImGui_ImplSDL2_Shutdown();
    }
}
