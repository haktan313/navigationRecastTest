#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "NavSystem/NavigationSystem.h"
#include "Scene.h"
#include "Shader.h"
#include "NavSystem/NavigationSystemDebugTools.h"

class Application
{
public:
    Application();
    ~Application();
    
    void Run();
private:
    void CalculateDeltaTime();
    bool Init();
    void Shutdown();

    void Render();
    void RenderImGui();

    void InputManager();
    static void SizeCallback(GLFWwindow* window, int width, int height);
    static void MouseCallback(GLFWwindow* window, double xposIn, double yposIn);
    
    GLFWwindow* m_Window;
    Shader* m_Shader;
    Scene* m_Scene;
    NavigationSystem* m_NavSystem;

    Camera m_Camera;

    float m_DeltaTime, m_LastFrame;
    float m_LastX, m_LastY;
    bool m_bFirstMouse;
    
    static Application* s_Instance;
};
