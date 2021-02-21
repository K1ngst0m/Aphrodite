#include "test.h"

float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f,  0.5f, 0.0f
};


auto vertexShaderSource = "#version 330 core\n"
                          "layout (location = 0) in vec3 aPos;\n"
                          "void main()\n"
                          "{\n"
                          "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                          "}\0";

auto fragmentShaderSource = "#version 330 core\n"
                            "out vec4 FragColor;\n"
                            "void main()\n"
                            "{\n"
                            "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
                            "}\n\0";

unsigned int fragmentShader;
unsigned int vertexShader;
unsigned int shaderProgram;
unsigned int VBO, VAO;
GLFWwindow * m_window;

int main() {
    // 初始化glfw
    GLFW_Init();
    mainWindowInit();
    ShaderInit();
    draw();
    Update();
    ApplicationEnd();
    return 0;
}

// 窗口变化回调函数
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
//    std::cout << "framebuffer_size_callback" << std::endl;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        std::cout << "press escape key, close window" << std::endl;
        glfwSetWindowShouldClose(window, true);
    }
}

void Update(){
    // 检查GLFW是否要求退出
    while (!glfwWindowShouldClose(m_window)) {
        // 输入
        processInput(m_window);

        // 渲染指令
        glClearColor(.2, .3, .3, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 检查触发事件
        glfwPollEvents();
        // 交换颜色缓冲, 用于将缓存绘制到屏幕上
        glfwSwapBuffers(m_window);
    }
}

void GLFW_Init() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); //
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void draw(){
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 设置顶点指针
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(VAO);
}

void VertexShader(){
    // 着色器编译
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success){
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
}

void FragmentShader(){
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success){
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
}

void ShaderLink(){
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

}

void ShaderInit(){
    VertexShader();
    FragmentShader();
    ShaderLink();
}

void mainWindowInit(){
    // 创建窗口
    m_window = glfwCreateWindow(800, 600, "LearnOpenGL", nullptr, nullptr);
    if(m_window == nullptr){
        std::cout << "glfw create window error" << std::endl;
        glfwTerminate();
    }

    // 设置窗口上下文为当前线程上下文
    glfwMakeContextCurrent(m_window);

    // 初始化glad
    auto gllSuccess = gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    if(!gllSuccess){
        std::cout << "glad load error" << std::endl;
        glfwTerminate();
    }

    // 定义视口
    glViewport(0, 0, 800, 600);

    glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
}

void ApplicationEnd(){
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
}