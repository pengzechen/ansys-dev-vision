


cmake_minimum_required(VERSION 3.10)
project(ImGui_GLAD_OpenGL)
set(CMAKE_CXX_STANDARD 17)

# ImGui 和 glad 头文件路径
include_directories(
    libs/glad/include
    libs/imgui
    libs/imgui/backends
    libs/glfw/include
    libs/glm        
)

# ImGui 源码文件
set(IMGUI_SRC
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)



# 使用 MSYS2 的 GLFW & OpenGL 库
find_package(OpenGL REQUIRED)

add_subdirectory(libs/glfw)  # 加入 GLFW 子目录



add_executable(app
    # src/helloworld.cpp
    # src/main-1.1.4.cpp    # 三维的一个正方体
    # src/main-1.2.0.cpp    # 图形的颜色大小可变化  
    # src/main-1.2.1.cpp    # model view projection
    # src/main-1.2.2.cpp    # mvp 控制
    # src/main-1.2.3.cpp    # 有了 camera 
    # src/main-1.2.4.cpp    # 封装了 camera 类
    src/main-1.2.5.cpp    # 封装了 camera Control 类
    libs/glad/src/glad.c
    ${IMGUI_SRC}
)

target_link_libraries(app glfw OpenGL::GL)



# cmake .. -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM="D:\Qt\Tools\mingw1120_64\bin\make.exe"