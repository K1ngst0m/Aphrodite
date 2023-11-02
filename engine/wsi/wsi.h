#ifndef WSI_H_
#define WSI_H_

#include "app/input/event.h"
#include "common/common.h"
#include "api/vulkan/vkUtils.h"

namespace aph::vk
{
class Instance;
}

struct WSICreateInfo
{
    uint32_t width;
    uint32_t height;
    bool     enableUI;
};

namespace aph
{

class WSI
{
protected:
    WSI(const WSICreateInfo& createInfo) :
        m_width{createInfo.width},
        m_height(createInfo.height),
        m_enabledUI(createInfo.enableUI)
    {
        init();
    }

public:
    static std::unique_ptr<WSI> Create(const WSICreateInfo& createInfo)
    {
        CM_LOG_INFO("Init window: [%d, %d]", createInfo.width, createInfo.height);
        return std::unique_ptr<WSI>(new WSI(createInfo));
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

    void*    m_window    = {};
    uint32_t m_width     = {};
    uint32_t m_height    = {};
    bool     m_enabledUI = {};
};

}  // namespace aph

#endif  // WSI_H_
