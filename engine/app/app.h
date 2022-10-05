#ifndef VULKANBASE_H_
#define VULKANBASE_H_

#include <string>

namespace vkl {
class BaseApp {
public:
    BaseApp(std::string sessionName = "Untitled");
    virtual ~BaseApp() = default;

    virtual void init() = 0;
    virtual void run() = 0;
    virtual void finish() = 0;

protected:
    const std::string m_sessionName;
};
} // namespace vkl

#endif // VULKANBASE_H_
