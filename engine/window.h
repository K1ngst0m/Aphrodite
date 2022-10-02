#ifndef WINDOW_H_
#define WINDOW_H_

#include "common.h"

class GLFWwindow;

namespace vkl {

struct WindowData {
    GLFWwindow *window = nullptr;
    uint32_t    width;
    uint32_t    height;
    bool        resized = false;
    WindowData(uint32_t w, uint32_t h)
        : width(w), height(h) {
    }
    float getAspectRatio() const {
        return static_cast<float>(width) * height;
    }
};

using FramebufferSizeFunc = std::function<void(int width, int height)>;
using CursorPosFunc       = std::function<void(double xposIn, double yposIn)>;
using KeyFunc             = std::function<void(int key, int scancode, int action, int mods)>;

class Window {
public:
    void init(uint32_t width, uint32_t height);
    void cleanup();

    std::shared_ptr<WindowData> getWindowData();
    GLFWwindow                 *getHandle();
    uint32_t                    getWidth();
    uint32_t                    getHeight();
    float                       getAspectRatio();

public:
    void setFramebufferSizeCallback(const FramebufferSizeFunc &cbFunc);
    void setCursorPosCallback(const CursorPosFunc &cbFunc);
    void setKeyCallback(const KeyFunc &cbFunc);
    void setWidth(uint32_t w);
    void setHeight(uint32_t h);
    void setCursorVisibility(bool flag);
    void toggleCurosrVisibility();
    void close();
    bool shouldClose();
    void pollEvents();

private:
    bool                        isCursorVisible = false;
    std::shared_ptr<WindowData> _data           = nullptr;
    FramebufferSizeFunc         _framebufferResizeCB;
    CursorPosFunc               _cursorPosCB;
    KeyFunc                     _keyCB;
};
} // namespace vkl

#endif // WINDOW_H_
