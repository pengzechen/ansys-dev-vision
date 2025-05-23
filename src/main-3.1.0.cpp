



// c++ 标准库
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include <map>
#include <algorithm>
#include <array>
#include <cassert>
#include <set>

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
#include <glm/ext/scalar_common.hpp>



class XdmfMeshLoader {
public:
    struct MixedElement {
        uint8_t type;
        std::vector<uint64_t> conn;
    };

    std::vector<std::array<double, 3>> geometry;
    std::vector<MixedElement> mixedTopology;

    std::unordered_map<std::string, std::vector<int>> nodeAttributes;
    std::unordered_map<std::string, std::vector<int>> cellAttributes;

    void Load(const std::string& xdmfFilePath);

private:
    void ParseDataItem(tinyxml2::XMLElement* dataItem, std::string& hdf5Path, std::vector<hsize_t>& dims);
    void LoadGeometry(const std::string& hdf5Path, hsize_t numPoints);
    void LoadMixedTopology(const std::string& hdf5Path);
    int GetNodeCountForXdmfType(uint8_t type);
};


void XdmfMeshLoader::Load(const std::string& xdmfFilePath) {
    tinyxml2::XMLDocument doc;
    auto ret = doc.LoadFile(xdmfFilePath.c_str());
    if (ret != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("Failed to load XDMF file.");
    }

    tinyxml2::XMLElement* root = doc.RootElement(); // Xdmf
    if (!root) throw std::runtime_error("XDMF root element not found");

    tinyxml2::XMLElement* domain = root->FirstChildElement("Domain");
    if (!domain) throw std::runtime_error("Domain element not found");

    tinyxml2::XMLElement* grid = domain->FirstChildElement("Grid");
    if (!grid) throw std::runtime_error("Grid element not found");

    // 解析 Geometry 节点
    tinyxml2::XMLElement* geometry = grid->FirstChildElement("Geometry");
    if (!geometry) throw std::runtime_error("Geometry element not found");

    std::string geomHdf5Path;
    std::vector<hsize_t> geomDims;
    tinyxml2::XMLElement* geomDataItem = geometry->FirstChildElement("DataItem");
    if (!geomDataItem) throw std::runtime_error("Geometry DataItem not found");

    ParseDataItem(geomDataItem, geomHdf5Path, geomDims);

    if (geomDims.size() != 2 || geomDims[1] != 3) {
        throw std::runtime_error("Geometry dimensions invalid, expected Nx3.");
    }
    LoadGeometry(geomHdf5Path, geomDims[0]);

    // 解析 Topology 节点
    tinyxml2::XMLElement* topology = grid->FirstChildElement("Topology");
    if (!topology) throw std::runtime_error("Topology element not found");

    const char* topoType = topology->Attribute("TopologyType");
    if (!topoType || std::string(topoType) != "Mixed") {
        throw std::runtime_error("Only Mixed topology supported.");
    }

    tinyxml2::XMLElement* topoDataItem = topology->FirstChildElement("DataItem");
    if (!topoDataItem) throw std::runtime_error("Topology DataItem not found");

    std::string topoHdf5Path;
    std::vector<hsize_t> topoDims;
    ParseDataItem(topoDataItem, topoHdf5Path, topoDims);

    LoadMixedTopology(topoHdf5Path);
}

void XdmfMeshLoader::ParseDataItem(tinyxml2::XMLElement* dataItem, std::string& hdf5Path, std::vector<hsize_t>& dims) {
    const char* dimsStr = dataItem->Attribute("Dimensions");
    if (!dimsStr) throw std::runtime_error("DataItem Dimensions attribute missing.");

    // 解析 Dimensions 格式 "30574 3" 为 vector<hsize_t>
    dims.clear();
    std::stringstream ss(dimsStr);
    std::string token;
    while (ss >> token) {
        dims.push_back(static_cast<hsize_t>(std::stoull(token)));
    }

    const char* format = dataItem->Attribute("Format");
    if (!format || std::string(format) != "HDF") {
        throw std::runtime_error("Only HDF format supported.");
    }

    const char* text = dataItem->GetText();
    if (!text) throw std::runtime_error("DataItem text missing (expected HDF file path and dataset).");

    // text格式一般是 "../data/model_3d.h5:/data0"
    std::string fullStr(text);
    size_t pos = fullStr.find(':');
    if (pos == std::string::npos) throw std::runtime_error("HDF5 dataset path invalid.");

    hdf5Path = fullStr.substr(0, pos) + ":" + fullStr.substr(pos + 1);
}

void XdmfMeshLoader::LoadGeometry(const std::string& hdf5Path, hsize_t numPoints) {
    // hdf5Path 格式: "../data/model_3d.h5:/data0"
    size_t pos = hdf5Path.find(':');
    if (pos == std::string::npos) throw std::runtime_error("Invalid hdf5Path in LoadGeometry");

    std::string fileName = hdf5Path.substr(0, pos);
    std::string datasetName = hdf5Path.substr(pos + 1);

    // 使用 HDF5 API 读取数据到 geometry 中
    hid_t file_id = H5Fopen(fileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id < 0) throw std::runtime_error("Failed to open HDF5 file for geometry.");

    hid_t dataset_id = H5Dopen(file_id, datasetName.c_str(), H5P_DEFAULT);
    if (dataset_id < 0) {
        H5Fclose(file_id);
        throw std::runtime_error("Failed to open dataset for geometry.");
    }

    geometry.resize(numPoints);

    // 创建内存空间与文件空间
    hsize_t dims[2] = {numPoints, 3};
    hid_t memspace = H5Screate_simple(2, dims, NULL);

    // 读取数据
    herr_t status = H5Dread(dataset_id, H5T_NATIVE_DOUBLE, memspace, H5S_ALL, H5P_DEFAULT, geometry.data());
    if (status < 0) {
        H5Dclose(dataset_id);
        H5Fclose(file_id);
        H5Sclose(memspace);
        throw std::runtime_error("Failed to read geometry data.");
    }

    H5Dclose(dataset_id);
    H5Fclose(file_id);
    H5Sclose(memspace);
}

void XdmfMeshLoader::LoadMixedTopology(const std::string& hdf5Path) {
    // hdf5Path 格式: "../data/model_3d.h5:/data1"
    size_t pos = hdf5Path.find(':');
    if (pos == std::string::npos) throw std::runtime_error("Invalid hdf5Path in LoadMixedTopology");

    std::string fileName = hdf5Path.substr(0, pos);
    std::string datasetName = hdf5Path.substr(pos + 1);

    // 打开文件和数据集
    hid_t file_id = H5Fopen(fileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id < 0) throw std::runtime_error("Failed to open HDF5 file for topology.");

    hid_t dataset_id = H5Dopen(file_id, datasetName.c_str(), H5P_DEFAULT);
    if (dataset_id < 0) {
        H5Fclose(file_id);
        throw std::runtime_error("Failed to open dataset for topology.");
    }

    // 获取数据集尺寸，读取一维 int64_t 数组
    hid_t space = H5Dget_space(dataset_id);
    int ndims = H5Sget_simple_extent_ndims(space);
    if (ndims != 1) {
        H5Sclose(space);
        H5Dclose(dataset_id);
        H5Fclose(file_id);
        throw std::runtime_error("Topology data should be 1D.");
    }

    hsize_t dims;
    H5Sget_simple_extent_dims(space, &dims, NULL);

    // 读取全部数据
    std::vector<int64_t> rawData(dims);
    herr_t status = H5Dread(dataset_id, H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT, rawData.data());
    if (status < 0) {
        H5Sclose(space);
        H5Dclose(dataset_id);
        H5Fclose(file_id);
        throw std::runtime_error("Failed to read topology data.");
    }

    H5Sclose(space);
    H5Dclose(dataset_id);
    H5Fclose(file_id);

    // 清空旧数据
    mixedTopology.clear();

    // Mixed拓扑格式：
    // 数据中，前一个元素是类型标识符 type，
    // 后面紧跟该单元的节点索引，节点个数通过 type 得到
    

    size_t i = 0;
    while (i < rawData.size()) {
        int type = static_cast<int>(rawData[i]);
        int nodeCount = GetNodeCountForXdmfType(type);
        if (nodeCount <= 0) {
            // Debug use
            for (size_t k = i - 20; k < std::min<size_t>(i + 20, rawData.size()); ++k) {
                std::cout << "rawData[" << k << "] = " << rawData[k] << std::endl;
            }
            throw std::runtime_error("Unknown XDMF element type: " + std::to_string(type));
        }

        if (i + nodeCount >= rawData.size()) {
            throw std::runtime_error("Topology data corrupted or incomplete.");
        }

        MixedElement elem;
        elem.type = type;
        elem.conn.resize(nodeCount);
        for (int j = 0; j < nodeCount; ++j) {
            elem.conn[j] = static_cast<uint64_t>(rawData[i + 1 + j]);
        }
        mixedTopology.push_back(std::move(elem));

        i += 1 + nodeCount;
    }
}

int XdmfMeshLoader::GetNodeCountForXdmfType(uint8_t type) {
    // 参考XDMF规范，常用单元类型及节点数
    switch(type) {
        case 1: return 2;  // (线)
        case 2: return 3;  // (三角形)
        case 4: return 3;  // 
        case 5: return 4;  // (四边形)
        case 6: return 4;  // (四面体)
        case 7: return 5;  // (五面体)
        case 8: return 6;  // Prism (Wedge)
        case 9: return 8;  // Hexahedron (6面体)
        case 36: return 6; // 带有中点的三角形 2d

        default: return -1; // 未知类型
    }
}

// 默认摄像机参数
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  500.0f;
const float SENSITIVITY =  0.5f;
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

class Mesh {
public:
    unsigned int VAO, VBO, EBO;
    XdmfMeshLoader loader;

    std::vector<float> vertices;                 // 每个顶点包含 6 个 float（位置 + 法线）
    std::vector<unsigned int> triangle_indices;  // 三角面索引
    std::vector<unsigned int> line_indices;      // 线索引

    void mesh_face() {
        std::unordered_map<uint64_t, int> indexMap;
        std::vector<float> tempVertices;
        std::vector<unsigned int> tempIndices;

        const auto& geom = loader.geometry;

        for (const auto& elem : loader.mixedTopology) {
            const auto& conn = elem.conn;

            if (elem.type == 9 && conn.size() == 8) {  // HEX8
                const int hexFaces[6][4] = {
                    {0, 1, 2, 3}, {4, 5, 6, 7}, {0, 4, 5, 1},
                    {3, 7, 6, 2}, {0, 3, 7, 4}, {1, 5, 6, 2}
                };
                for (const auto& face : hexFaces) {
                    uint64_t a = conn[face[0]];
                    uint64_t b = conn[face[1]];
                    uint64_t c = conn[face[2]];
                    uint64_t d = conn[face[3]];

                    for (auto vid : {a, b, c, a, c, d}) {
                        if (indexMap.count(vid) == 0) {
                            indexMap[vid] = static_cast<int>(tempVertices.size() / 3);
                            tempVertices.insert(tempVertices.end(), {
                                static_cast<float>(geom[vid][0]),
                                static_cast<float>(geom[vid][1]),
                                static_cast<float>(geom[vid][2])
                            });
                        }
                        tempIndices.push_back(indexMap[vid]);
                    }
                }
            } else if (elem.type == 8 && conn.size() == 6) {  // WEDGE6
                const int wedgeFaces[5][3] = {
                    {0, 1, 2}, {3, 4, 5}, {0, 1, 4}, {1, 2, 5}, {2, 0, 3}
                };
                for (const auto& face : wedgeFaces) {
                    uint64_t a = conn[face[0]];
                    uint64_t b = conn[face[1]];
                    uint64_t c = conn[face[2]];

                    for (auto vid : {a, b, c}) {
                        if (indexMap.count(vid) == 0) {
                            indexMap[vid] = static_cast<int>(tempVertices.size() / 3);
                            tempVertices.insert(tempVertices.end(), {
                                static_cast<float>(geom[vid][0]),
                                static_cast<float>(geom[vid][1]),
                                static_cast<float>(geom[vid][2])
                            });
                        }
                        tempIndices.push_back(indexMap[vid]);
                    }
                }
            }
        }

        vertices = std::move(tempVertices);
        triangle_indices = std::move(tempIndices);

        // OpenGL: setup VAO / VBO / EBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangle_indices.size() * sizeof(unsigned int), triangle_indices.data(), GL_STATIC_DRAW);

        // vertex position attribute only
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    void mesh_line() {

        std::cout << "Loaded geometry points count: " << loader.geometry.size() << std::endl;
        std::cout << "Loaded mixed topology elements count: " << loader.mixedTopology.size() << std::endl;

        // === 顶点数据 ===
        for (const auto& pt : loader.geometry) {
            vertices.push_back(static_cast<float>(pt[0]));
            vertices.push_back(static_cast<float>(pt[1]));
            vertices.push_back(static_cast<float>(pt[2]));
        }

        // === 拓扑线框处理 ===
        for (const auto& elem : loader.mixedTopology) {
            const auto& conn = elem.conn;
            if (elem.type == 8 && conn.size() == 6) {
                // WEDGE6: 三棱柱
                AddEdges(line_indices, conn, {
                    {0,1}, {1,2}, {2,0},       // bottom triangle
                    {3,4}, {4,5}, {5,3},       // top triangle
                    {0,3}, {1,4}, {2,5}        // vertical edges
                });
            } else if (elem.type == 9 && conn.size() == 8) {
                // HEX8: 六面体立方体
                AddEdges(line_indices, conn, {
                    {0,1}, {1,2}, {2,3}, {3,0}, // bottom
                    {4,5}, {5,6}, {6,7}, {7,4}, // top
                    {0,4}, {1,5}, {2,6}, {3,7}  // sides
                });
            } else {
                // std::cerr << "Skipping unsupported element type: " << static_cast<int>(elem.type) << " with " << conn.size() << " nodes\n";
            }
        }

        // === OpenGL Buffer ===
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, line_indices.size() * sizeof(unsigned int), line_indices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    Mesh(XdmfMeshLoader& loader, bool wireframe) : loader(loader) {
        if (wireframe) 
        mesh_line();
        else
        mesh_face();
    }

    void draw_triangle() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(triangle_indices.size()), GL_UNSIGNED_INT, 0);
    }

    void draw_line() const {
        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, static_cast<GLsizei>(line_indices.size()), GL_UNSIGNED_INT, 0);
    }

    ~Mesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
private:
    void AddEdges(std::vector<unsigned int>& indices, const std::vector<uint64_t>& conn, const std::initializer_list<std::pair<int,int>>& edges) {
        for (auto [i, j] : edges) {
            indices.push_back(static_cast<unsigned int>(conn[i]));
            indices.push_back(static_cast<unsigned int>(conn[j]));
        }
    }
};


const char* vertexShaderSource = R"glsl(
#version 330 core
    layout(location = 0) in vec3 aPos;
    uniform mat4 uMVP;
    void main() {
        gl_Position = uMVP * vec4(aPos, 1.0);
    }
)glsl";


const char* fragmentShaderSource = R"glsl(
#version 330 core
    out vec4 FragColor;
    uniform vec3 uColor;
    void main() {
        FragColor = vec4(uColor, 1.0);  // 红褐色
    }
)glsl";


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

    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
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
    Camera(glm::vec3 position = glm::vec3(-50.0f, 50.0f, 50.0f), 
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
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspectRatio, 0.1f, 100000.0f);
        return projection * view * model;
    }
};




void imgui_init(Application &app);
void imgui_draw();

// 帧间隔时间
float deltaTime = 0.0f; 
float lastFrame = 0.0f;

Camera camera;


int main(int argc, char** argv) {
    Application app;
    if (!app.init(1600, 1200, "Dynamic Vertex Color Demo")) return -1;

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
    glfwSetCursorPosCallback(app.window, [](GLFWwindow* window, double xpos, double ypos) {
        auto* controller = static_cast<CameraController*>(glfwGetWindowUserPointer(window));
        if (controller) {
            controller->onMouseMove(xpos, ypos);
        }
    });

    glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // 隐藏光标并锁定到窗口中央

    glfwSwapInterval(1);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);  // 默认是 GL_LESS，有时换成 LEQUAL 更稳

    // glEnable(GL_CULL_FACE);          // 启用面剔除
    // glCullFace(GL_BACK);             // 指定剔除背面（默认就是 GL_BACK）
    // glFrontFace(GL_CW);              // 指定逆时针为正面（OpenGL 默认是 GL_CCW）

        
    Shader shader(vertexShaderSource, fragmentShaderSource);

    XdmfMeshLoader loader;
    loader.Load("model_big.xdmf");
    Mesh mesh_line(loader, true);
    Mesh mesh_face(loader, false);

    glLineWidth(2.0f);  // 默认是1像素，太细也会导致看起来像断线
    
    float time = 0.0f;

    while (!app.shouldClose()) {
        float currentFrame = float(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        controller.onKey(app.window, deltaTime);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);  // 设置底色
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // 清屏，用底色覆盖整个窗口, 启用深度测试

        MVPBuilder mvpBuilder;
        
        /* 现在不希望他旋转 */
        // glm::mat4 mvp = mvpBuilder.rotate(time * 0.5f, {0, 0, 1})
        //                 .build(camera, 800.0f / 600.0f);
        glm::mat4 mvp = mvpBuilder.build(camera, 800.0f / 600.0f);

        // 这几行要保证顺序
        // mesh.updateVertices(time);
        shader.use();
        shader.setMat4("uMVP", mvp);

        shader.setVec3("uColor", glm::vec3(0.0f, 0.0f, 0.0f));
        mesh_face.draw_triangle();

        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonOffset(-1.0f, -1.0f);  // 负值让线“浮”在表面上
        shader.setVec3("uColor", glm::vec3(1.0f, 1.0f, 1.0f));
        mesh_line.draw_line();
        glDisable(GL_POLYGON_OFFSET_LINE);
        
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
