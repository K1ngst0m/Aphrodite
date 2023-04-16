#ifndef UIRENDERER_H_
#define UIRENDERER_H_

#include "common/common.h"

namespace aph
{
class WindowData;
class IUIRenderer
{
public:
    IUIRenderer(std::shared_ptr<WindowData> windowData) : m_windowData(std::move(windowData)) {}

protected:
    std::shared_ptr<WindowData> m_windowData = {};
};
}  // namespace aph

#endif  // UIRENDERER_H_
