

/*
   效果： 绘制一个矩形

   是1.1的一个封装。封装了 Shader 和 mesh 和 Application
*/


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

// 窗口大小变化回调
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// 简单输入处理，ESC退出
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// ---------------------- Application 类 -------------------------
class Application {
public:
    GLFWwindow* window = nullptr;

    bool init(int width = 800, int height = 600, const char* title = "OpenGL Demo") {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW\n";
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

        window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Failed to initialize GLAD\n";
            return false;
        }

        return true;
    }

    bool shouldClose() const {
        return glfwWindowShouldClose(window);
    }

    void swapBuffers() {
        glfwSwapBuffers(window);
    }

    void pollEvents() {
        glfwPollEvents();
    }

    void terminate() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

// ----------------------- Shader 类 -------------------------------
class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexSrc, const char* fragmentSrc) {
        // 编译顶点着色器
        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexSrc, nullptr);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // 编译片元着色器
        unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentSrc, nullptr);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // 链接程序
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // 删除着色器对象
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() const {
        glUseProgram(ID);
    }

private:
    void checkCompileErrors(unsigned int shader, const std::string& type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                          << infoLog << "\n";
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                          << infoLog << "\n";
            }
        }
    }
};

// ------------------------ Mesh 类 -------------------------------
class Mesh {
public:
    unsigned int VAO, VBO, EBO;

    Mesh() {
        // 顶点数据：位置(x,y) + 颜色(r,g,b)
        float vertices[] = {
            0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  // 右上角，红色
            0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  // 右下角，绿色
           -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  // 左下角，蓝色
           -0.5f,  0.5f,  1.0f, 1.0f, 0.0f   // 左上角，黄色
        };

        unsigned int indices[] = {
            0, 1, 3,
            1, 2, 3
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // 位置属性
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // 颜色属性
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    ~Mesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

    void draw() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
};

// -------------------- 着色器源码 ------------------------------
const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

out vec3 ourColor;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    ourColor = aColor;
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
in vec3 ourColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(ourColor, 1.0);
}
)glsl";

// ----------------------- 主函数 -------------------------------
int main() {
    Application app;
    if (!app.init(800, 600, "Mesh Class Demo")) return -1;

    Shader shader(vertexShaderSource, fragmentShaderSource);
    Mesh mesh;

    while (!app.shouldClose()) {
        processInput(app.window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        mesh.draw();

        app.swapBuffers();
        app.pollEvents();
    }

    app.terminate();
    return 0;
}
