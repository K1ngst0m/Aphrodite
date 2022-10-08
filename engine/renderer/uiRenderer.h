#ifndef UIRENDERER_H_
#define UIRENDERER_H_

#include "common/common.h"

namespace vkl {
class WindowData;
class UIRenderer {
public:
    UIRenderer(const std::shared_ptr<WindowData>& windowData)
        : _windowData(windowData) {
    }
    virtual void initUI() = 0;
    virtual void drawUI() = 0;

protected:
    std::shared_ptr<WindowData> _windowData = nullptr;
};
} // namespace vkl

#endif // UIRENDERER_H_
