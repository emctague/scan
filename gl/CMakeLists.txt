cmake_minimum_required(VERSION 3.13)
project(scan)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(scan scan.cpp)
target_link_libraries(scan ${GLEW_LIBRARIES} glfw  ${glm_LIBRARIES} ${OPENGL_LIBRARY})
target_link_directories(scan PUBLIC glfw)
target_include_directories(scan PUBLIC glfw ${GLEW_INCLUDE_DIRS} ${glm_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})

