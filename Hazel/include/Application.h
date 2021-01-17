//
// Created by Npchitman on 2021/1/17.
//

#ifndef HAZELENGINE_APPLICATION_H
#define HAZELENGINE_APPLICATION_H

#include "Core.h"
namespace Hazel{
    class HAZEL_API Application{
    public:
        Application();
        virtual ~Application();
        void Run();
    };

    // To be defined in CLIENT
    Application* CreateApplication();
}

#endif //HAZELENGINE_APPLICATION_H
