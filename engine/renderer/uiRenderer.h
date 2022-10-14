#ifndef UIRENDERER_H_
#define UIRENDERER_H_

#include "common/common.h"

namespace vkl {
class WindowData;
class UIRenderer {
public:
    UIRenderer(std::shared_ptr<WindowData>  windowData)
        : _windowData(std::move(windowData)) {
    }

protected:
    std::shared_ptr<WindowData> _windowData = nullptr;
};
} // namespace vkl

#endif // UIRENDERER_H_
