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
    bool        resized = { false };
    WindowData(uint32_t w, uint32_t h) : width{ w }, height{ h } {}
    float getAspectRatio() const { return static_cast<float>(width) / height; }
};

struct CursorData
{
    float lastX           = {};
    float lastY           = {};
    bool  firstMouse      = { true };
    bool  isCursorDisable = { false };
    bool  isCursorVisible = { false };
    CursorData(float lastXin, float lastYin) : lastX{ lastXin }, lastY{ lastYin } {}
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
    void                        toggleCurosrVisibility() { setCursorVisibility(!m_cursorData->isCursorVisible); }
    float                       getCursorYpos() { return m_cursorData->lastY; }
    float                       getCursorXpos() { return m_cursorData->lastX; }
    float                       getAspectRatio() { return m_windowData->getAspectRatio(); }
    GLFWwindow*                 getHandle() { return m_windowData->window; }
    void                        setHeight(uint32_t h) { m_windowData->height = h; }
    void                        setWidth(uint32_t w) { m_windowData->width = w; }
    uint32_t                    getWidth() { return m_windowData->width; }
    uint32_t                    getHeight() { return m_windowData->height; }

public:
    void setFramebufferSizeCallback(const FramebufferSizeFunc& cbFunc);
    void setCursorPosCallback(const CursorPosFunc& cbFunc);
    void setKeyCallback(const KeyFunc& cbFunc);
    void setMouseButtonCallback(const MouseButtonFunc& cbFunc);
    void setCursorVisibility(bool flag);
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
