#ifndef VULKANBASE_H_
#define VULKANBASE_H_

#include "vkl.hpp"

#include <chrono>
namespace vkl {

class Timer{
public:
    Timer(float& interval)
        :_interval(interval)
    {
        start = std::chrono::steady_clock::now();
    }
    ~Timer(){
        auto end = std::chrono::steady_clock::now();
        _interval = (start - end).count();
    }

private:
    std::chrono::steady_clock::time_point start;
    float& _interval;
};

class vklApp {
public:
    vklApp(std::string sessionName = "Untitled");
    virtual ~vklApp() = default;

public:
    void init();
    void run();
    void finish();

protected:
    void initWindow();
    void initRenderer();
    void cleanup();

protected:
    virtual void initDerive() = 0;
    virtual void drawFrame()  = 0;

protected:
    virtual void keyboardHandleDerive(int key, int scancode, int action, int mods);
    virtual void mouseHandleDerive(double xposIn, double yposIn);

protected:
    const std::string m_sessionName;

    float m_deltaTime;

    std::shared_ptr<Window>   m_window;
    std::shared_ptr<Camera>   m_defaultCamera = nullptr;
    std::unique_ptr<Renderer> m_renderer;

    vkl::DeletionQueue m_deletionQueue;
};
} // namespace vkl

#endif // VULKANBASE_H_
