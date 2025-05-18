

/*
    实现简单的视角控制（比如键盘或鼠标控制摄像机）。
*/

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// ImGui 头文件
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

// ----------- 摄像机参数 ---------------
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw   = -90.0f;   // 水平方向，初始看向负Z
float pitch = 0.0f;     // 垂直方向

float lastX = 400, lastY = 300; // 鼠标上次位置（屏幕中心）
bool firstMouse = true;

// 摄像机移动速度和鼠标灵敏度
float movementSpeed = 2.5f;
float mouseSensitivity = 0.5f;

// 帧间隔时间
float deltaTime = 0.0f; 
float lastFrame = 0.0f;

float fov = 45.0f;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = float(xpos);
        lastY = float(ypos);
        firstMouse = false;
    }

    float xoffset = float(xpos) - lastX;
    float yoffset = lastY - float(ypos); // y 反向，鼠标向上视角往上看

    lastX = float(xpos);
    lastY = float(ypos);

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if(pitch > 89.0f) pitch = 89.0f;
    if(pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void processInput(GLFWwindow* window) {
    float currentFrame = float(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    float velocity = movementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += velocity * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= velocity * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += velocity * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= velocity * cameraUp;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    fov -= float(yoffset);
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 90.0f) fov = 90.0f;
}

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

void imgui_init(Application &app);
void imgui_draw();

int main() {
    Application app;
    if (!app.init(800, 600, "Dynamic Vertex Color Demo")) return -1;

    imgui_init(app);

    // glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // 隐藏光标并锁定到窗口中央
    // glfwSetCursorPosCallback(app.window, mouse_callback);            // 设置鼠标回调
    // glfwSetScrollCallback(app.window, scroll_callback);              // 设置鼠标回调

    Shader shader(vertexShaderSource, fragmentShaderSource);
    Mesh mesh;

    float time = 0.0f;

    glfwSwapInterval(1);

    while (!app.shouldClose()) {
        processInput(app.window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // mesh.updateVertices(time);
        glm::mat4 projection = glm::perspective(glm::radians(fov), 800.0f/600.0f, 0.1f, 100.0f);

        // 视图矩阵，摄像机控制视角
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        // 模型矩阵：旋转 + 平移 + 缩放
        glm::mat4 model = glm::mat4(1.0f);
        // model = glm::translate(model, glm::vec3(0.2f * sin(time), 0.0f, 0.0f)); // 左右平移
        model = glm::rotate(model, time * 0.5f, glm::vec3(0.0f, 0.0f, 1.0f));  // 旋转
        // model = glm::scale(model, glm::vec3(1.0f + 0.3f * sin(time), 1.0f, 1.0f));
        glm::mat4 mvp = projection * view * model;

        shader.use();
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
        mesh.draw();
        imgui_draw();

        time += 0.1f;
        app.swapBuffers();
        app.pollEvents();
    }

    return 0;
}


void imgui_init(Application &app) {
    // 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(app.window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark();
}

void imgui_draw() {
    // 🔧 ImGui 每帧开始
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();  // 👈 很重要！
    // ====================================================================
    ImGui::Begin("Camera Debug");

    ImGui::SliderFloat("FOV", &fov, 1.0f, 90.0f);
    ImGui::SliderFloat3("Position", glm::value_ptr(cameraPos),      -10.0f, 10.0f);
    ImGui::SliderFloat3("Front",    glm::value_ptr(cameraFront),    -1.0f, 1.0f);
    ImGui::SliderFloat3("Up",       glm::value_ptr(cameraUp),       -1.0f, 1.0f);  // ✅ 添加这一行

    // ✅ 添加 yaw 和 pitch 控制
    bool changed = false;
    changed |= ImGui::SliderFloat("Yaw", &yaw, -180.0f, 180.0f);
    changed |= ImGui::SliderFloat("Pitch", &pitch, -89.0f, 89.0f);

    // ✅ 实时更新 cameraFront 向量
    if (changed) {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }

    if (ImGui::Button("Reset Camera Vectors")) {
        cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
        cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
        cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);
        yaw = -90.0f;
        pitch = 0.0f;
        fov = 45.0f;
    }
    // ====================================================================
    ImGui::End();

    // 渲染 ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
