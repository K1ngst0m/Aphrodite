#include "triangle.h"

void triangle::initDerive() {
    buildCommands();
}

void triangle::drawFrame() {
    m_renderer->prepareFrame();
    m_renderer->submitFrame();
}

void triangle::buildCommands() {

}

int main() {
    triangle app;

    app.vkl::vklApp::init();
    app.vkl::vklApp::run();
    app.vkl::vklApp::finish();
}
