#include "triangle_demo.h"

triangle_demo::triangle_demo()
    : vkl::BaseApp("triangle_demo") {
}

void triangle_demo::init() {
    // setup window
    m_window = vkl::Window::Create(1366, 768);

    // renderer config
    vkl::RenderConfig config{
        .enableDebug         = true,
        .enableUI            = false,
        .initDefaultResource = true,
        .maxFrames           = 2,
    };

    // setup renderer
    m_renderer = vkl::VulkanRenderer::Create(&config, m_window->getWindowData());

    // build draw command
    m_renderer->drawDemo();
}

void triangle_demo::run() {
    // get frame deltatime
    auto timer = vkl::Timer(m_deltaTime);

    // loop
    while (!m_window->shouldClose()) {
        m_window->pollEvents();
        m_renderer->renderOneFrame();
    }
}

void triangle_demo::finish() {
    // wait device idle before cleanup
    m_renderer->idleDevice();
    m_renderer->destroy();
}

int main() {
    triangle_demo app;

    app.init();
    app.run();
    app.finish();
}
