

/*
    矩形： 颜色变化，大小变化； 左右平移，旋转，缩放(mvp)
*/

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


// 简单输入处理，ESC退出
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}


// 窗口大小变化回调
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

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


class Mesh {
public:
    unsigned int VAO, VBO, EBO;
    float vertices[20];  // 4 顶点 * (2位置 + 3颜色)

    Mesh() {
        // 初始化为矩形中心为原点 [-0.5, 0.5]
        float initVertices[] = {
            0.5f,  0.5f,  1.0f, 1.0f, 1.0f,  // 右上
            0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  // 右下
           -0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  // 左下
           -0.5f,  0.5f,  1.0f, 1.0f, 1.0f   // 左上
        };
        memcpy(vertices, initVertices, sizeof(initVertices));

        unsigned int indices[] = {
            0, 1, 3,
            1, 2, 3
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void updateVertices(float t) {
        // 正方形大小与动画参数
        float baseSize = 0.5f;
        float scale = 0.2f + 0.1f * std::sin(t * 0.5f);
        float positions[8] = {
            baseSize * scale,  baseSize * scale,   // 右上
            baseSize * scale, -baseSize * scale,   // 右下
           -baseSize * scale, -baseSize * scale,   // 左下
           -baseSize * scale,  baseSize * scale    // 左上
        };

        for (int i = 0; i < 4; ++i) {
            vertices[i * 5 + 0] = positions[i * 2 + 0];  // x
            vertices[i * 5 + 1] = positions[i * 2 + 1];  // y

            // 动态颜色：每个顶点有不同相位的正弦波
            vertices[i * 5 + 2] = (std::sin(t + i) * 0.5f) + 0.5f;     // R
            vertices[i * 5 + 3] = (std::sin(t + i + 2.0f) * 0.5f) + 0.5f; // G
            vertices[i * 5 + 4] = (std::sin(t + i + 4.0f) * 0.5f) + 0.5f; // B
        }

        // 更新整个 VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    void draw() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    ~Mesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};

const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec3 aColor;

    uniform mat4 uMVP;

    out vec3 ourColor;

    void main() {
        gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
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


int main() {
    Application app;
    if (!app.init(800, 600, "Dynamic Vertex Color Demo")) return -1;



    Shader shader(vertexShaderSource, fragmentShaderSource);
    Mesh mesh;

    float time = 0.0f;

    glfwSwapInterval(1);

    while (!app.shouldClose()) {
        processInput(app.window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        mesh.updateVertices(time);

        // 初始化正交投影和视图矩阵
        glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -0.75f, 0.75f, -1.0f, 1.0f);
        glm::mat4 view = glm::mat4(1.0f); // 相机不动
        // 模型矩阵：旋转 + 平移 + 缩放
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.2f * sin(time), 0.0f, 0.0f)); // 左右平移
        model = glm::rotate(model, time * 0.5f, glm::vec3(0.0f, 0.0f, 1.0f));  // 旋转
        model = glm::scale(model, glm::vec3(1.0f + 0.3f * sin(time), 1.0f, 1.0f));
        glm::mat4 mvp = projection * view * model;

        shader.use();
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));

        mesh.draw();

        time += 0.1f;

        app.swapBuffers();
        app.pollEvents();
    }

    return 0;
}
