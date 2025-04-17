#include "wsi.h"
#include "imgui_impl_sdl3.h"

#include "api/vulkan/instance.h"
#include "common/profiler.h"
#include "event/eventManager.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

using namespace aph;

static auto SDLKeyCast(SDL_Keycode key) -> Key
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

auto WindowSystem::Create(const WindowSystemCreateInfo& createInfo) -> Expected<WindowSystem*>
{
    APH_PROFILER_SCOPE();
    CM_LOG_INFO("Init window: [%d, %d]", createInfo.width, createInfo.height);

    // Create window system with minimal initialization
    auto* pWindowSystem = new WindowSystem(createInfo);
    if (!pWindowSystem)
    {
        return { Result::RuntimeError, "Failed to allocate WindowSystem instance" };
    }

    // Complete the initialization process
    Result initResult = pWindowSystem->initialize(createInfo);
    if (!initResult.success())
    {
        delete pWindowSystem;
        return { initResult.getCode(), initResult.toString() };
    }

    return pWindowSystem;
}

void WindowSystem::Destroy(WindowSystem* pWindowSystem)
{
    if (!pWindowSystem)
    {
        return;
    }

    APH_PROFILER_SCOPE();

    if (pWindowSystem->m_window)
    {
        SDL_DestroyWindow(static_cast<SDL_Window*>(pWindowSystem->m_window));
    }

    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();

    delete pWindowSystem;
}

Result WindowSystem::initialize(const WindowSystemCreateInfo& createInfo)
{
    APH_PROFILER_SCOPE();

    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        APH_ASSERT(false);
        return { Result::RuntimeError, "Failed to initialize SDL" };
    }

    // Create window with appropriate flags for high DPI if enabled
    uint32_t windowFlags = SDL_WINDOW_VULKAN;
    if (m_enableHighDPI)
    {
        // SDL3 uses SDL_WINDOW_HIGH_PIXEL_DENSITY for high DPI support
        windowFlags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;
    }

    m_window = (void*)SDL_CreateWindow("Aphrodite Engine", m_width, m_height, windowFlags);

    if (m_window == nullptr)
    {
        CM_LOG_ERR("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return { Result::RuntimeError, "Failed to create SDL window" };
    }

    // Update DPI scale after window creation
    updateDPIScale();

    return Result::Success;
}

auto WindowSystem::getWidth() const -> uint32_t
{
    return m_width;
}

auto WindowSystem::getHeight() const -> uint32_t
{
    return m_height;
}

auto WindowSystem::isHighDPIEnabled() const -> bool
{
    return m_enableHighDPI;
}

auto WindowSystem::getSurface(vk::Instance* instance) -> ::vk::SurfaceKHR
{
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface((SDL_Window*)m_window, instance->getHandle(), vk::vkAllocator(), &surface);
    return surface;
}

auto WindowSystem::update() -> bool
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (ImGui::GetCurrentContext())
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
        }
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            return false;
        case SDL_EVENT_KEY_DOWN:
        {
            KeyState state{};
            auto keysym = event.key.key;
            auto gkey   = SDLKeyCast(keysym);

            switch (event.key.type)
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
                    visible             = !visible;
                }
                else
                {
                    m_eventManager.pushEvent(KeyboardEvent{ gkey, state });
                }
            }
            break;
            case SDL_EVENT_KEY_UP:
            {
                state = KeyState::Released;
                m_eventManager.pushEvent(KeyboardEvent{ gkey, state });
            }
            break;
            default:
                break;
            }

            if (event.key.repeat)
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
            lastX        = x;
            lastY        = y;

            m_eventManager.pushEvent(MouseMoveEvent{ deltaX, deltaY, x, y });
        }
        break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            MouseButton btn;
            switch (event.button.button)
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

            m_eventManager.pushEvent(MouseButtonEvent{ btn, x, y, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN });
        }
        break;
        case SDL_EVENT_WINDOW_RESIZED:
        {
            resize(event.window.data1, event.window.data2);

            WindowResizeEvent resizeEvent{ static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) };

            // Push the event to your event queue or handle it immediately
            m_eventManager.pushEvent(resizeEvent);
        }
        break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        {
            // Handle pixel size change event (specific to SDL3)
            updateDPIScale();
            WindowResizeEvent resizeEvent{ static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) };
            m_eventManager.pushEvent(resizeEvent);
        }
        break;
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        {
            // Handle display scale change event (specific to SDL3)
            CM_LOG_INFO("SDL3 display scale changed event received");
            updateDPIScale();
        }
        break;
        default:
            break;
        }
    }

    m_eventManager.processAll();

    return true;
}

void WindowSystem::close()
{
}

void WindowSystem::resize(uint32_t width, uint32_t height)
{
    m_width  = width;
    m_height = height;

    int w, h;
    auto window = (SDL_Window*)m_window;
    SDL_GetWindowSize(window, &w, &h);

    if (w != m_width || h != m_height)
    {
        SDL_SetWindowSize(window, m_width, m_height);
    }

    // Update DPI scale after resize
    updateDPIScale();
}

auto WindowSystem::getRequiredExtensions() -> SmallVector<const char*>
{
    SmallVector<const char*> extensions{};
    {
        uint32_t extensionCount;
        auto ptr = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
        extensions.assign(ptr, ptr + extensionCount);
    }
    return extensions;
}

auto WindowSystem::getNativeHandle() -> void*
{
    return m_window;
}

auto WindowSystem::getPixelWidth() const -> uint32_t
{
    int width, height;
    SDL_GetWindowSizeInPixels((SDL_Window*)m_window, &width, &height);
    return static_cast<uint32_t>(width);
}

auto WindowSystem::getPixelHeight() const -> uint32_t
{
    int width, height;
    SDL_GetWindowSizeInPixels((SDL_Window*)m_window, &width, &height);
    return static_cast<uint32_t>(height);
}

auto WindowSystem::getDPIScale() const -> float
{
    return m_dpiScale;
}

void WindowSystem::updateDPIScale()
{
    if (!m_window)
    {
        m_dpiScale = 1.0f;
        return;
    }

    if (m_enableHighDPI)
    {
        auto* window = (SDL_Window*)m_window;

        // In SDL3, window display scale combines content scale and pixel density
        float scale           = SDL_GetWindowDisplayScale(window);
        uint32_t displayIndex = SDL_GetDisplayForWindow(window);

        // If scale is invalid, calculate using separate components
        if (scale <= 0.0f)
        {
            // Get display content scale (what SDL3 docs call the display's expected scale)
            float contentScale = 1.0f;

            if (displayIndex >= 0)
            {
                contentScale = SDL_GetDisplayContentScale(displayIndex);
            }

            // Get window pixel density (ratio between pixel size and window size)
            float pixelDensity = SDL_GetWindowPixelDensity(window);

            // Combine both factors
            scale = contentScale * pixelDensity;
        }

        // Ensure minimum scale of 1.0
        float newScale = std::max(1.0f, scale);

        // Check if the scale has changed significantly
        if (std::abs(newScale - m_dpiScale) > 0.01f)
        {
            // Update scale
            m_dpiScale = newScale;

            // Get current window and pixel dimensions
            uint32_t pixelWidth  = getPixelWidth();
            uint32_t pixelHeight = getPixelHeight();

            // Log the current DPI configuration
            CM_LOG_INFO("DPI scale changed: display scale=%.2f, content scale=%.2f, pixel density=%.2f", m_dpiScale,
                        SDL_GetDisplayContentScale(displayIndex), SDL_GetWindowPixelDensity(window));

            // Emit DPI change event
            m_eventManager.pushEvent(DPIChangeEvent(m_dpiScale, m_width, m_height, pixelWidth, pixelHeight));
        }
    }
    else
    {
        m_dpiScale = 1.0f;
    }
}

aph::WindowSystem::WindowSystem(const WindowSystemCreateInfo& createInfo)
    : m_width{ createInfo.width }
    , m_height(createInfo.height)
    , m_enableHighDPI(createInfo.enableHighDPI)
{
}
