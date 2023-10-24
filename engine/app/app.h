#ifndef VULKANBASE_H_
#define VULKANBASE_H_

#include <string>

namespace aph
{
class BaseApp
{
public:
    BaseApp(std::string sessionName = "Untitled");
    virtual ~BaseApp() = default;

    virtual void init()   = 0;
    virtual void load()   = 0;
    virtual void run()    = 0;
    virtual void unload() = 0;
    virtual void finish() = 0;

protected:
    const std::string m_sessionName;
};
}  // namespace aph

#endif  // VULKANBASE_H_
