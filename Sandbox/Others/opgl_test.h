//
// Created by Npchitman on 2021/1/21.
//

#ifndef HAZELENGINE_OPGL_TEST_H
#define HAZELENGINE_OPGL_TEST_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void GLFW_Init();
void mainWindowInit();
void draw();
void ShaderInit();
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void ApplicationEnd();
void Update();
#endif //HAZELENGINE_OPGL_TEST_H
