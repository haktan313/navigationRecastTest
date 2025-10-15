
cmake_minimum_required(VERSION 3.15)
project(NavMeshDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB OUR_SOURCES "source/*.cpp" "source/*.h")

file(GLOB SHADER_FILES "source/shaders/*.vert" "source/shaders/*.frag")

add_library(glfw INTERFACE)
target_include_directories(glfw SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw-3.4.bin.WIN64/include)

target_link_libraries(glfw INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw-3.4.bin.WIN64/lib-vc2022/glfw3.lib)

add_library(glad STATIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/glad/include)

add_library(glm INTERFACE)
target_include_directories(glm SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libs/glm-1.0.2)

add_executable(NavMeshDemo ${OUR_SOURCES} ${SHADER_FILES})

target_include_directories(NavMeshDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)

target_link_libraries(NavMeshDemo PRIVATE glfw glad glm opengl32 gdi32)