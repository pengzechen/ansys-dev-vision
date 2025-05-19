


cmake_minimum_required(VERSION 3.10)
project(ImGui_GLAD_OpenGL)
set(CMAKE_CXX_STANDARD 17)


# set(HDF5_ROOT "C:/Program Files/HDF_Group/HDF5/1.14.6")

# 使用 MSYS2 的 GLFW & OpenGL 库
find_package(OpenGL REQUIRED) 
find_package(HDF5 REQUIRED)

get_target_property(OpenGL_GL_INTERFACE_INCLUDE OpenGL::GL INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "OpenGL::GL include path: ${OpenGL_GL_INTERFACE_INCLUDE}")
message(STATUS "HDF5_INCLUDE_DIRS: ${HDF5_INCLUDE_DIRS}")
message(STATUS "HDF5_LIBRARIES: ${HDF5_LIBRARIES}")

# ImGui 和 glad 头文件路径
include_directories(
    libs/glad/include
    libs/imgui
    libs/imgui/backends
    libs/glfw/include
    libs/glm        
    ${HDF5_INCLUDE_DIRS} 
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

add_subdirectory(libs/glfw)  # 加入 GLFW 子目录

add_executable(app
    # src/helloworld.cpp
    # src/main-1.1.4.cpp    # 三维的一个正方体
    # src/main-1.2.0.cpp    # 图形的颜色大小可变化  
    # src/main-1.2.1.cpp    # model view projection
    # src/main-1.2.2.cpp    # mvp 控制
    # src/main-1.2.3.cpp    # 有了 camera 
    # src/main-1.2.4.cpp    # 封装了 camera 类
    # src/main-1.2.5.cpp    # 封装了 camera Control 类
    src/hdf5-test.cpp
    libs/glad/src/glad.c
    ${IMGUI_SRC}
)

target_link_libraries(app glfw OpenGL::GL HDF5::HDF5)



# cmake .. -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM="D:\Qt\Tools\mingw1120_64\bin\make.exe"