

/*
    实现简单的视角控制（比如键盘或鼠标控制摄像机）。  封装 Camera 类
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

// 默认摄像机参数
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;


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

    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
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

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
public:
    // 摄像机属性
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // 欧拉角
    float Yaw;
    float Pitch;
    // 配置参数
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // 构造函数（使用向量参数）
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), 
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
           float yaw = YAW, float pitch = PITCH)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
          MovementSpeed(SPEED),
          MouseSensitivity(SENSITIVITY),
          Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // 构造函数（使用标量参数）
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
          MovementSpeed(SPEED),
          MouseSensitivity(SENSITIVITY),
          Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // 返回视图矩阵
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // 处理键盘输入
    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
        if (direction == UP)
            Position += WorldUp * velocity;
        if (direction == DOWN)
            Position -= WorldUp * velocity;
    }

    // 处理鼠标位移输入，constrainPitch 为 true 时限制垂直角度避免屏幕翻转
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        if (constrainPitch) {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // 更新 Front、Right 和 Up 向量
        updateCameraVectors();
    }

    // 处理鼠标滚轮输入，用于缩放视野
    void ProcessMouseScroll(float yoffset) {
        Zoom -= yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

private:
    // 根据最新的欧拉角更新 Front、Right 和 Up 向量
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // 重新计算 Right 和 Up 向量
        Right = glm::normalize(glm::cross(Front, WorldUp));  
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

Camera camera;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static float lastX = 400.0f;
    static float lastY = 300.0f;
    static bool firstMouse = true;

    if (firstMouse) {
        lastX = float(xpos);
        lastY = float(ypos);
        firstMouse = false;
    }

    float xoffset = float(xpos) - lastX;
    float yoffset = lastY - float(ypos); // 注意反转 yoffset
    lastX = float(xpos);
    lastY = float(ypos);

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll((float)yoffset);
}

void processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

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

// 帧间隔时间
float deltaTime = 0.0f; 
float lastFrame = 0.0f;


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
        float currentFrame = float(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(app.window, deltaTime);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);  // 设置底色
        glClear(GL_COLOR_BUFFER_BIT);           // 清屏，用底色覆盖整个窗口

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 800.0f/600.0f, 0.1f, 100.0f);
        // 视图矩阵，摄像机控制视角
        glm::mat4 view = camera.GetViewMatrix();
        // 模型矩阵：旋转 + 平移 + 缩放
        glm::mat4 model = glm::mat4(1.0f);
        // model = glm::translate(model, glm::vec3(0.2f * sin(time), 0.0f, 0.0f));      // 左右平移
        model = glm::rotate(model, time * 0.5f, glm::vec3(0.0f, 0.0f, 1.0f));           // 旋转
        // model = glm::scale(model, glm::vec3(1.0f + 0.3f * sin(time), 1.0f, 1.0f));   // 缩放
        glm::mat4 mvp = projection * view * model;

        // 这几行要保证顺序
        mesh.updateVertices(time);
        shader.use();
        shader.setMat4("uMVP", mvp);
        mesh.draw();
        
        // 绘制窗口的gui
        imgui_draw();

        time += deltaTime;
        app.swapBuffers();
        app.pollEvents();
    }

    app.terminate();
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

    ImGui::SliderFloat("FOV", &camera.Zoom, 1.0f, 90.0f);
    ImGui::SliderFloat3("Position", glm::value_ptr(camera.Position),      -10.0f, 10.0f);
    ImGui::SliderFloat3("Front",    glm::value_ptr(camera.Front),    -1.0f, 1.0f);
    ImGui::SliderFloat3("Up",       glm::value_ptr(camera.Up),       -1.0f, 1.0f);  // ✅ 添加这一行

    // ✅ 添加 yaw 和 pitch 控制
    bool changed = false;
    changed |= ImGui::SliderFloat("Yaw", &camera.Yaw, -180.0f, 180.0f);
    changed |= ImGui::SliderFloat("Pitch", &camera.Pitch, -89.0f, 89.0f);

    // ✅ 实时更新 cameraFront 向量
    if (changed) {
        glm::vec3 front;
        front.x = cos(glm::radians(camera.Yaw)) * cos(glm::radians(camera.Pitch));
        front.y = sin(glm::radians(camera.Pitch));
        front.z = sin(glm::radians(camera.Yaw)) * cos(glm::radians(camera.Pitch));
        camera.Front = glm::normalize(front);
    }

    if (ImGui::Button("Reset Camera Vectors")) {
        camera.Position   = glm::vec3(0.0f, 0.0f, 3.0f);
        camera.Front = glm::vec3(0.0f, 0.0f, -1.0f);
        camera.Up    = glm::vec3(0.0f, 1.0f, 0.0f);
        camera.Yaw = -90.0f;
        camera.Pitch = 0.0f;
        camera.Zoom = 45.0f;
    }
    // ====================================================================
    ImGui::End();

    // 渲染 ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
