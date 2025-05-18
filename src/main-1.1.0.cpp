
/*
   效果： 绘制一个矩形，由两个三角型组成
*/

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

// 顶点着色器源码
const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

out vec3 ourColor;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    ourColor = aColor;
}
)glsl";

// 片元着色器源码
const char* fragmentShaderSource = R"glsl(
#version 330 core
in vec3 ourColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(ourColor, 1.0);
}
)glsl";

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main()
{
    // 初始化 GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // 创建 OpenGL 3.3 Core Profile 上下文
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // MacOS需要
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "VBO + VAO + EBO Demo", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    

    

    unsigned int VBO, VAO, EBO;

    // 生成并绑定 VAO 操作手册
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    
    // 生成并绑定 VBO
    // 顶点数据：位置(x,y) + 颜色(r,g,b)  5个float
    float vertices[] = {
         0.5f,  0.5f,  1.0f, 0.0f, 0.0f, // 右上角，红色
         0.5f, -0.5f,  0.0f, 1.0f, 0.0f, // 右下角，绿色
        -0.5f, -0.5f,  0.0f, 0.0f, 1.0f, // 左下角，蓝色
        -0.5f,  0.5f,  1.0f, 1.0f, 0.0f  // 左上角，黄色
    };
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    // 生成并绑定 EBO （加工工艺步骤单
    // 索引数据：两三角形组成矩形
    unsigned int indices[] = {
        0, 1, 3, // 第一个三角形
        1, 2, 3  // 第二个三角形
    };
    // 生成并绑定 EBO
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);



    // 设置顶点属性指针，位置属性
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 设置顶点属性指针，颜色属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 解绑 VAO（可选，但最好解绑以避免误操作）
    glBindVertexArray(0);

    unsigned int shaderProgram;
    {
        // 编译顶点着色器
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
        glCompileShader(vertexShader);

        // 检查编译错误
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            std::cerr << "ERROR: Vertex shader compilation failed\n" << infoLog << "\n";
        }

        // 编译片元着色器
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
        glCompileShader(fragmentShader);

        // 检查编译错误
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            std::cerr << "ERROR: Fragment shader compilation failed\n" << infoLog << "\n";
        }

        // 创建着色器程序并链接
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        // 检查链接错误
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
            std::cerr << "ERROR: Shader program linking failed\n" << infoLog << "\n";
        }

        // 删除着色器对象，链接后不需要了
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    // 渲染循环
    while (!glfwWindowShouldClose(window))
    {
        // 输入处理（这里简单，ESC关闭）
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // 清空屏幕颜色缓冲
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 使用着色器程序绘制矩形
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // 交换缓冲区和处理事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理资源
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
