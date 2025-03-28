#include "imgui_impl_sdl3.h"
#include "wsi.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "api/vulkan/instance.h"
#include "event/eventManager.h"

using namespace aph;

static Key SDLKeyCast(SDL_Keycode key)
{
#define k(sdlk, aph)  \
    case SDLK_##sdlk: \
        return Key::aph
    switch (key)
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
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        CM_LOG_ERR("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        APH_ASSERT(false);
    }

    // Create window
    m_window = (void*)SDL_CreateWindow("Aphrodite Engine", m_width, m_height, SDL_WINDOW_VULKAN);

    if (m_window == nullptr)
    {
        CM_LOG_ERR("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        APH_ASSERT(false);
    }

    m_pEventManager = std::make_unique<EventManager>();
}

::vk::SurfaceKHR WindowSystem::getSurface(vk::Instance* instance)
{
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface((SDL_Window*)m_window, instance->getHandle(), vk::vkAllocator(), &surface);
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
    while (SDL_PollEvent(&windowEvent))
    {
        switch (windowEvent.type)
        {
        case SDL_EVENT_QUIT:
            return false;
        case SDL_EVENT_KEY_DOWN:
        {
            KeyState state{};
            auto keysym = windowEvent.key.key;
            auto gkey = SDLKeyCast(keysym);

            switch (windowEvent.key.type)
            {
            case SDL_EVENT_KEY_DOWN:
            {
                state = KeyState::Pressed;
                if (gkey == Key::Escape)
                {
                    close();
                    return false;
                }

                if (gkey == Key::_1)
                {
                    // TODO cursor visible
                    static bool visible = false;
                    visible = !visible;
                }
                else
                {
                    m_pEventManager->pushEvent(KeyboardEvent{ gkey, state });
                }
            }
            break;
            case SDL_EVENT_KEY_UP:
            {
                state = KeyState::Released;
                m_pEventManager->pushEvent(KeyboardEvent{ gkey, state });
            }
            break;
            default:
                break;
            }

            if (windowEvent.key.repeat)
            {
                state = KeyState::Repeat;
            }
        }
        break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            float x, y;
            SDL_GetMouseState(&x, &y);

            static float lastX = getWidth() / 2.0f;
            static float lastY = getHeight() / 2.0f;

            float deltaX = lastX - x;
            float deltaY = lastY - y;
            lastX = x;
            lastY = y;

            m_pEventManager->pushEvent(MouseMoveEvent{ deltaX, deltaY, x, y });
        }
        break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            MouseButton btn;
            switch (windowEvent.button.button)
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

            float x, y;
            SDL_GetMouseState(&x, &y);

            m_pEventManager->pushEvent(MouseButtonEvent{ btn, x, y, windowEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN });
        }
        break;
        case SDL_EVENT_WINDOW_RESIZED:
        {
            resize(windowEvent.window.data1, windowEvent.window.data2);

            WindowResizeEvent resizeEvent{ static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) };

            // Push the event to your event queue or handle it immediately
            m_pEventManager->pushEvent(resizeEvent);
        }
        break;
        default:
            break;
        }
    }

    m_pEventManager->processAll();

    if (m_enabledUI)
    {
        ImGui_ImplSDL3_NewFrame();
    }

    return true;
};

void WindowSystem::close() {
    // glfwSetWindowShouldClose((GLFWwindow*)m_window, true);
};

void WindowSystem::resize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    int w, h;
    auto window = (SDL_Window*)m_window;
    SDL_GetWindowSize(window, &w, &h);

    if (w != m_width || h != m_height)
    {
        SDL_SetWindowSize(window, m_width, m_height);
    }
}

SmallVector<const char*> WindowSystem::getRequiredExtensions()
{
    SmallVector<const char*> extensions{};
    {
        uint32_t extensionCount;
        auto ptr = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
        extensions.assign(ptr, ptr + extensionCount);
    }
    return extensions;
}

bool WindowSystem::initUI()
{
    if (m_enabledUI)
    {
        return ImGui_ImplSDL3_InitForVulkan((SDL_Window*)m_window);
    }
    return false;
};

void WindowSystem::deInitUI() const
{
    if (m_enabledUI)
    {
        ImGui_ImplSDL3_Shutdown();
    }
}
