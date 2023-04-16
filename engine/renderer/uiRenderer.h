#ifndef UIRENDERER_H_
#define UIRENDERER_H_

#include <utility>

#include "common/common.h"

namespace aph
{
class Window;
class IUIRenderer
{
public:
    IUIRenderer(std::shared_ptr<Window> window) : m_window(std::move(window)) {}

protected:
    std::shared_ptr<Window> m_window = {};
};
}  // namespace aph

#endif  // UIRENDERER_H_
