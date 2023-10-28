#ifndef WSI_H_
#define WSI_H_

#include "app/input/event.h"
#include "common/common.h"
#include "api/vulkan/vkUtils.h"

namespace aph::vk
{
class Instance;
}

namespace aph
{

class WSI
{
protected:
    WSI(uint32_t width, uint32_t height) : m_width{width}, m_height(height) { init(); }

public:
    static std::unique_ptr<WSI> Create(uint32_t width = 800, uint32_t height = 600)
    {
        CM_LOG_INFO("Init window: [%d, %d]", width, height);
        return std::unique_ptr<WSI>(new WSI(width, height));
    }

    virtual ~WSI();

public:
    bool     initUI();
    void     deInitUI();
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }
    void     resize(uint32_t width, uint32_t height);

    std::vector<const char*> getRequiredExtensions();

    VkSurfaceKHR getSurface(vk::Instance* instance);

    bool update();
    void close();

protected:
    void init();

    void*    m_window = {};
    uint32_t m_width  = {};
    uint32_t m_height = {};
};

}  // namespace aph

#endif  // WSI_H_
