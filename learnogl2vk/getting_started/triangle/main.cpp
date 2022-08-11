#include <iostream>
#include "vulkanBase.h"

int main() {
    vkl::vkBase app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
