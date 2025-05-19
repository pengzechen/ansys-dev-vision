

// c++ 标准库
#include <iostream>
#include <vector>
#include <string>

// 加载数据使用
#include "tinyxml2.h"
#include "H5public.h"
#include "hdf5.h"


// ============ openGL 和 窗口库 ================
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// ImGui 头文件
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


// 简化表示 Geometry 数据
struct GeometryData {
    std::string hdf5_path; // e.g. "disk_2d.h5:/data0"
    int num_points = 0;
    int dim = 0;
};

// 简化表示 Topology 数据
struct TopologyData {
    std::string hdf5_path; // e.g. "disk_2d.h5:/data1"
    int num_elements = 0;
    int nodes_per_element = 0;
};

// 网格数据结构
struct MeshData {
    GeometryData geometry;
    TopologyData topology;
};

// XdmfMeshLoader 类
class XdmfMeshLoader {
public:
    bool Load(const std::string& xdmf_filename);

    const MeshData& GetMeshData() const { return mesh; }

    std::vector<double> ReadGeometryData() const;
    std::vector<uint64_t> ReadTopologyData() const;

private:
    MeshData mesh;

    static bool ParseDataItem(tinyxml2::XMLElement* dataItemElem, std::string& out_path, int& out_dim0, int& out_dim1);

    static std::pair<std::string, std::string> ParseHDF5Path(const std::string& full_path);

    static std::vector<double> ReadPoints(const std::string& filename, const std::string& dataset, int num_points, int dim);
    static std::vector<uint64_t> ReadIndices(const std::string& filename, const std::string& dataset, int num_elements, int nodes_per_element);
};

// 读取并解析 XDMF 文件
bool XdmfMeshLoader::Load(const std::string& xdmf_filename) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(xdmf_filename.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to load XML file\n";
        return false;
    }

    auto root = doc.RootElement();
    if (!root || std::string(root->Name()) != "Xdmf") {
        std::cerr << "Invalid XDMF root element\n";
        return false;
    }

    auto domain = root->FirstChildElement("Domain");
    auto grid = domain ? domain->FirstChildElement("Grid") : nullptr;
    if (!grid) {
        std::cerr << "Missing Domain or Grid\n";
        return false;
    }

    // Geometry
    auto geometry = grid->FirstChildElement("Geometry");
    if (!geometry) {
        std::cerr << "No Geometry element\n";
        return false;
    }
    auto geom_data_item = geometry->FirstChildElement("DataItem");
    std::string geom_path;
    int geom_d0 = 0, geom_d1 = 0;
    if (!ParseDataItem(geom_data_item, geom_path, geom_d0, geom_d1)) return false;
    mesh.geometry = {geom_path, geom_d0, geom_d1};

    // Topology
    auto topology = grid->FirstChildElement("Topology");
    auto topo_data_item = topology ? topology->FirstChildElement("DataItem") : nullptr;
    if (!topology || !topo_data_item) {
        std::cerr << "Missing Topology or its DataItem\n";
        return false;
    }

    int num_elements = 0;
    int nodes_per_element = 0;
    topology->QueryIntAttribute("NumberOfElements", &num_elements);
    topology->QueryIntAttribute("NodesPerElement", &nodes_per_element);

    std::string topo_path;
    int topo_d0 = 0, topo_d1 = 0;
    if (!ParseDataItem(topo_data_item, topo_path, topo_d0, topo_d1)) return false;

    if (topo_d0 != num_elements || topo_d1 != nodes_per_element) {
        std::cerr << "Topology dimensions mismatch\n";
        return false;
    }

    mesh.topology = {topo_path, num_elements, nodes_per_element};
    return true;
}

bool XdmfMeshLoader::ParseDataItem(tinyxml2::XMLElement* dataItemElem, std::string& out_path, int& out_dim0, int& out_dim1) {
    if (!dataItemElem) return false;

    const char* format = dataItemElem->Attribute("Format");
    if (!format || std::string(format) != "HDF") {
        std::cerr << "Only HDF format supported.\n";
        return false;
    }

    const char* dim_str = dataItemElem->Attribute("Dimensions");
    if (!dim_str) {
        std::cerr << "No Dimensions attribute.\n";
        return false;
    }

    sscanf(dim_str, "%d %d", &out_dim0, &out_dim1);

    const char* text = dataItemElem->GetText();
    if (!text) return false;

    std::string path = text;
    size_t start = path.find_first_not_of(" \t\r\n");
    size_t end = path.find_last_not_of(" \t\r\n");
    out_path = path.substr(start, end - start + 1);

    return true;
}

std::pair<std::string, std::string> XdmfMeshLoader::ParseHDF5Path(const std::string& full_path) {
    size_t pos = full_path.find(":");
    if (pos == std::string::npos) throw std::runtime_error("Invalid HDF5 path: " + full_path);
    return {full_path.substr(0, pos), full_path.substr(pos + 1)};
}

std::vector<double> XdmfMeshLoader::ReadPoints(const std::string& filename, const std::string& dataset, int num_points, int dim) {
    std::vector<double> points(num_points * dim);
    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    hid_t dset = H5Dopen(file, dataset.c_str(), H5P_DEFAULT);
    H5Dread(dset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, points.data());
    H5Dclose(dset);
    H5Fclose(file);
    return points;
}

std::vector<uint64_t> XdmfMeshLoader::ReadIndices(const std::string& filename, const std::string& dataset, int num_elements, int nodes_per_element) {
    std::vector<uint64_t> indices(num_elements * nodes_per_element);
    hid_t file = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    hid_t dset = H5Dopen(file, dataset.c_str(), H5P_DEFAULT);
    H5Dread(dset, H5T_NATIVE_UINT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, indices.data());
    H5Dclose(dset);
    H5Fclose(file);
    return indices;
}

std::vector<double> XdmfMeshLoader::ReadGeometryData() const {
    auto [filename, dataset] = ParseHDF5Path(mesh.geometry.hdf5_path);
    return ReadPoints(filename, dataset, mesh.geometry.num_points, mesh.geometry.dim);
}

std::vector<uint64_t> XdmfMeshLoader::ReadTopologyData() const {
    auto [filename, dataset] = ParseHDF5Path(mesh.topology.hdf5_path);
    return ReadIndices(filename, dataset, mesh.topology.num_elements, mesh.topology.nodes_per_element);
}



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
    XdmfMeshLoader loader;

    std::vector<double> vertices;
    std::vector<uint64_t> quad_indices;
    std::vector<unsigned int> line_indices;

    Mesh(const std::string& xdmf_path = "disk_2d.xdmf") {
        if (!loader.Load(xdmf_path)) {
            std::cerr << "Failed to load mesh.\n";
            return;
        }

        const auto& mesh = loader.GetMeshData();
        if (mesh.topology.nodes_per_element != 4) {
            std::cerr << "Only 4-node quad elements are supported.\n";
            return;
        }

        int dim = mesh.geometry.dim;
        if (dim != 2 && dim != 3) {
            std::cerr << "Only 2D and 3D geometry supported.\n";
            return;
        }

        vertices = loader.ReadGeometryData();
        quad_indices = loader.ReadTopologyData();
        line_indices = QuadToLineIndices(quad_indices);

        // 转 float
        std::vector<float> float_vertices(vertices.begin(), vertices.end());

        // === OpenGL buffer ===
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, float_vertices.size() * sizeof(float), float_vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, line_indices.size() * sizeof(unsigned int), line_indices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, dim, GL_FLOAT, GL_FALSE, dim * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    void draw() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, static_cast<GLsizei>(line_indices.size()), GL_UNSIGNED_INT, 0);
    }

    ~Mesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

private:
    std::vector<unsigned int> QuadToLineIndices(const std::vector<uint64_t>& quads) {
        std::vector<unsigned int> lines;
        for (size_t i = 0; i < quads.size(); i += 4) {
            unsigned int a = quads[i];
            unsigned int b = quads[i + 1];
            unsigned int c = quads[i + 2];
            unsigned int d = quads[i + 3];

            lines.push_back(a); lines.push_back(b);
            lines.push_back(b); lines.push_back(c);
            lines.push_back(c); lines.push_back(d);
            lines.push_back(d); lines.push_back(a);
        }
        return lines;
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
    /*
     * WorldUp 是全局固定参考方向（通常是 y 轴），不参与变化，只用于计算。
     * Right 是相机右侧的方向，会根据 Front 动态更新。
     * Up 是相机“头顶”的方向，也动态更新，用来控制视角和视图矩阵。
    */
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

class CameraController {
    Camera* camera;
    float lastX, lastY;
    bool firstMouse;

public:
    CameraController()
        : camera(nullptr), lastX(400.0f), lastY(300.0f), firstMouse(true) {}

    void attach(Camera* cam) {
        camera = cam;
    }

    // 处理鼠标移动
    void onMouseMove(double xpos, double ypos) {
        if (!camera) return;

        if (firstMouse) {
            lastX = float(xpos);
            lastY = float(ypos);
            firstMouse = false;
        }

        float xoffset = float(xpos) - lastX;
        float yoffset = lastY - float(ypos);  // y轴反转
        lastX = float(xpos);
        lastY = float(ypos);

        camera->ProcessMouseMovement(xoffset, yoffset);
    }

    // 处理鼠标滚轮
    void onScroll(double xoffset, double yoffset) {
        if (!camera) return;
        camera->ProcessMouseScroll((float)yoffset);
    }

    // 处理键盘输入
    void onKey(GLFWwindow* window, float deltaTime) {
        if (!camera) return;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera->ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera->ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera->ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera->ProcessKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera->ProcessKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camera->ProcessKeyboard(DOWN, deltaTime);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }

    // 重置首次鼠标位置，外部可调用（例如切换窗口时）
    void resetMouse() {
        firstMouse = true;
    }
};

class MVPBuilder {
public:
    glm::mat4 model = glm::mat4(1.0f);

    MVPBuilder& rotate(float angleRad, glm::vec3 axis) {
        model = glm::rotate(model, angleRad, axis);
        return *this;
    }

    MVPBuilder& translate(glm::vec3 offset) {
        model = glm::translate(model, offset);
        return *this;
    }

    MVPBuilder& scale(glm::vec3 factor) {
        model = glm::scale(model, factor);
        return *this;
    }

    glm::mat4 build(Camera& camera, float aspectRatio) const {
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 100.0f);
        return projection * view * model;
    }
};


const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout(location = 0) in vec2 aPos;
    uniform mat4 uMVP;

    void main() {
        gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    }
)glsl";

const char* fragmentShaderSource = R"glsl(
    #version 330 core
    uniform vec3 uColor;  // 统一颜色
    out vec4 FragColor;

    void main() {
        FragColor = vec4(uColor, 1.0);
    }
)glsl";

void imgui_init(Application &app);
void imgui_draw();

// 帧间隔时间
float deltaTime = 0.0f; 
float lastFrame = 0.0f;

Camera camera;

int main() {
    Application app;
    if (!app.init(800, 600, "Dynamic Vertex Color Demo")) return -1;

    imgui_init(app);

    CameraController controller;
    controller.attach(&camera);
    
    glfwSetWindowUserPointer(app.window, &controller);

    // 设置 scroll 回调
    // glfwSetScrollCallback(app.window, scroll_callback);              // 设置鼠标回调
    // glfwSetScrollCallback(app.window, [](GLFWwindow* window, double xoffset, double yoffset) {
    //     // 获取用户指针，并强转回 controller 类型
    //     auto* controller = static_cast<CameraController*>(glfwGetWindowUserPointer(window));
    //     if (controller) {
    //         controller->onScroll(xoffset, yoffset);
    //     }
    // });
    // 设置 mouse move 回调
    // glfwSetCursorPosCallback(app.window, mouse_callback);            // 设置鼠标回调
    // glfwSetCursorPosCallback(app.window, [](GLFWwindow* window, double xpos, double ypos) {
    //     auto* controller = static_cast<CameraController*>(glfwGetWindowUserPointer(window));
    //     if (controller) {
    //         controller->onMouseMove(xpos, ypos);
    //     }
    // });

    // glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // 隐藏光标并锁定到窗口中央

    glfwSwapInterval(1);
    
    
    Shader shader(vertexShaderSource, fragmentShaderSource);
    
    Mesh mesh;
    
    float time = 0.0f;

    while (!app.shouldClose()) {
        float currentFrame = float(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        controller.onKey(app.window, deltaTime);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);  // 设置底色
        glClear(GL_COLOR_BUFFER_BIT);           // 清屏，用底色覆盖整个窗口

        MVPBuilder mvpBuilder;
        
        /* 现在不希望他旋转 */
        // glm::mat4 mvp = mvpBuilder.rotate(time * 0.5f, {0, 0, 1})
        //                 .build(camera, 800.0f / 600.0f);
        
        glm::mat4 mvp = mvpBuilder.build(camera, 800.0f / 600.0f);
                    

        // 这几行要保证顺序
        // mesh.updateVertices(time);
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
    // ImGui::SliderFloat3("Up",       glm::value_ptr(camera.Up),       -1.0f, 1.0f);  // ✅ 添加这一行

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
