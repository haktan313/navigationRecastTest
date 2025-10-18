
#include "Application.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "imgui.h"

Application* Application::s_Instance = nullptr;

Application::Application()
    : m_Window(nullptr), m_Shader(nullptr), m_Scene(nullptr), m_NavSystem(nullptr),
        m_Camera(), m_DeltaTime(0.0f), m_LastFrame(0.0f), m_LastX(640.0f), m_LastY(360.0f), m_bFirstMouse(true)
{
    s_Instance = this;
}

Application::~Application()
{
}

void Application::Run()
{
    if (!Init())
    {
        std::cout << "Application failed to initialize!" << std::endl;
        Shutdown();
        return;
    }
    m_Scene->SetupDefaultScene();
    while (!glfwWindowShouldClose(m_Window))
    {
        CalculateDeltaTime();
        InputManager();
        Render();

        glfwSwapBuffers(m_Window);
        glfwPollEvents();
    }
    Shutdown();
}

void Application::CalculateDeltaTime()
{
    float currentFrame = static_cast<float>(glfwGetTime());
    m_DeltaTime = currentFrame - m_LastFrame;
    m_LastFrame = currentFrame;
}

bool Application::Init()
{
    if (!glfwInit())
    {
        std::cout << "GLFW initialization failed!" << std::endl;
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(1280, 720, "NavMesh Example", NULL, NULL);
    if (m_Window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwSetWindowUserPointer(m_Window, this);
    glfwMakeContextCurrent(m_Window);
    glfwSetFramebufferSizeCallback(m_Window, Application::SizeCallback);
    glfwSetCursorPosCallback(m_Window, Application::MouseCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    glEnable(GL_DEPTH_TEST);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    m_Shader = new Shader("../source/shaders/simple.vert", "../source/shaders/simple.frag");
    m_Scene = new Scene();
    m_NavSystem = new NavigationSystem();
    
    return true;
}

void Application::Shutdown()
{
    delete m_NavSystem;
    m_NavSystem = nullptr;
    delete m_Scene;
    m_Scene = nullptr;
    delete m_Shader;
    m_Shader = nullptr;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_Window)
        glfwDestroyWindow(m_Window);
    m_Window = nullptr;
    
    glfwTerminate();
}

void Application::Render()
{
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (m_Scene && m_Shader)
    {
        m_Shader->use();
        glm::mat4 projection = glm::perspective(glm::radians(m_Camera.Zoom), 1280.0f / 720.0f, 0.1f, 100.0f);
        glm::mat4 view = m_Camera.GetViewMatrix();
        m_Shader->setMat4("projection", projection);
        m_Shader->setMat4("view", view);
        
        m_Scene->Render(m_Shader);
    }
    if (m_NavSystem && m_Shader)
        m_NavSystem->RenderDebugData(m_Camera,m_Shader, *m_Scene);
    
    RenderImGui();
}

void Application::RenderImGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    ImGui::Begin("Control Panel");
    if (ImGui::Button("Build NavMesh"))
        if (m_NavSystem && m_Scene)
            m_NavSystem->BuildNavMesh(*m_Scene);
    if (m_NavSystem) {
        const char* items[] = { "None", "Input Triangles", "Voxels (Solid)", "Walkable Surfaces", "Regions", "Connections", "Contours" };
        ImGui::Combo("Debug Draw", (int*)&m_NavSystem->m_DebugDrawMode, items, IM_ARRAYSIZE(items));
    }
    
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::InputManager()
{
    if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_Window, true);
    
    if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
        m_Camera.ProcessKeyboard(FORWARD, m_DeltaTime);
    if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
        m_Camera.ProcessKeyboard(BACKWARD, m_DeltaTime);
    if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
        m_Camera.ProcessKeyboard(LEFT, m_DeltaTime);
    if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
        m_Camera.ProcessKeyboard(RIGHT, m_DeltaTime);
    
    if (glfwGetKey(m_Window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS && glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Application::SizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void Application::MouseCallback(GLFWwindow* window, double xposIn, double yposIn)
{
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app)
        return;
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);
        if (app->m_bFirstMouse)
        {
            app->m_LastX = xpos;
            app->m_LastY = ypos;
            app->m_bFirstMouse = false;
        }
        float xoffset = xpos - app->m_LastX;
        float yoffset = app->m_LastY - ypos; 
        app->m_LastX = xpos;
        app->m_LastY = ypos;
        app->m_Camera.ProcessMouseMovement(xoffset, yoffset);
    }
}
