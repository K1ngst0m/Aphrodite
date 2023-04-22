#ifndef WINDOW_H_
#define WINDOW_H_

#include "common.h"
#include "inputCode.h"

class GLFWwindow;

namespace aph
{

struct WindowData
{
    GLFWwindow* window  = {};
    uint32_t    width   = {};
    uint32_t    height  = {};
    bool        resized = {false};
    WindowData(uint32_t w, uint32_t h) : width{w}, height{h} {}
};

struct CursorData
{
    float xPos            = {};
    float yPos            = {};
    bool  firstMouse      = {true};
    bool  isCursorDisable = {false};
    bool  isCursorVisible = {false};
    CursorData(float x, float y) : xPos{x}, yPos{y} {}
};

using FramebufferSizeFunc = std::function<void(int width, int height)>;
using CursorPosFunc       = std::function<void(double xposIn, double yposIn)>;
using KeyFunc             = std::function<void(int key, int scancode, int action, int mods)>;
using MouseButtonFunc     = std::function<void(int button, int action, int mods)>;

class Window
{
public:
    static std::shared_ptr<Window> Create(uint32_t width = 800, uint32_t height = 600);

    Window(uint32_t width, uint32_t height);
    ~Window();

public:
    std::shared_ptr<CursorData> getMouseData() { return m_cursorData; }
    std::shared_ptr<WindowData> getWindowData() { return m_windowData; }

    float    getAspectRatio() const { return static_cast<float>(m_windowData->width) / m_windowData->height; }
    uint32_t getCursorX() const { return m_cursorData->xPos; }
    uint32_t getCursorY() const { return m_cursorData->yPos; }
    uint32_t getWidth() const { return m_windowData->width; }
    uint32_t getHeight() const { return m_windowData->height; }
    uint32_t getKeyInputStatus(KeyId keycode);
    uint32_t getMouseButtonStatus(KeyId mouseButton);

    GLFWwindow* getHandle() { return m_windowData->window; }

public:
    void toggleCursorVisibility();
    void setCursorVisibility(bool flag);
    void setFramebufferSizeCallback(const FramebufferSizeFunc& cbFunc);
    void setCursorPosCallback(const CursorPosFunc& cbFunc);
    void setKeyCallback(const KeyFunc& cbFunc);
    void setMouseButtonCallback(const MouseButtonFunc& cbFunc);
    void close();
    bool shouldClose();
    void pollEvents();

private:
    std::shared_ptr<WindowData> m_windowData = {};
    std::shared_ptr<CursorData> m_cursorData = {};

    FramebufferSizeFunc m_framebufferResizeCB;
    CursorPosFunc       m_cursorPosCB;
    KeyFunc             m_keyCB;
    MouseButtonFunc     m_mouseButtonCB;
};
}  // namespace aph

#endif  // WINDOW_H_
