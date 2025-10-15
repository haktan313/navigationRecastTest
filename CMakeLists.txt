# CMake'in minimum versiyonunu ve proje adını belirt
cmake_minimum_required(VERSION 3.15)
project(NavMeshDemo)

# C++17 standardını kullan
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Kütüphane Yollarını Ayarla ---

# GLFW Kütüphanesi
add_library(glfw INTERFACE)
target_include_directories(glfw SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw-3.4.bin.WIN64/include)
target_link_libraries(glfw INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw-3.4.bin.WIN64/lib-vc2022/glfw3.lib)

# GLAD Kütüphanesi
add_library(glad STATIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/glad/include)

# GLM Kütüphanesi (Header-only)
add_library(glm INTERFACE)
target_include_directories(glm SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libs/glm-1.0.2)

# ImGui Kütüphanesi
set(IMGUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui-1.92.4/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui-1.92.4/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui-1.92.4/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui-1.92.4/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui-1.92.4/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui-1.92.4/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui-1.92.4)
target_link_libraries(imgui PUBLIC glfw)


# --- Ana Uygulamayı Oluştur ---

# DEĞİŞTİ: Artık main.cpp'nin yolunu 'source/' önekiyle belirtiyoruz
add_executable(NavMeshDemo source/main.cpp)

# YENİ: Projemizin header dosyalarını nerede bulacağını belirtiyoruz
target_include_directories(NavMeshDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/source)

# Gerekli tüm kütüphaneleri ve include dizinlerini ana uygulamamıza bağla
target_link_libraries(NavMeshDemo PRIVATE glfw glad glm imgui opengl32 gdi32)

# Başarılı olduğunda bir mesaj göster
message(STATUS "Proje hazır. 'build' klasörü oluşturup 'cmake ..' komutunu çalıştırın.")