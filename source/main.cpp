

#include "Shader.h"
#include "Camera.h"
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

struct Vec3f
{
    float x, y, z;
};
struct Triangle
{
    Vec3f verts[3];
};

std::vector<Triangle> g_InputTriangles;

unsigned int g_debugVao = 0;
unsigned int g_debugVbo = 0;


bool CreateWindow(GLFWwindow*& window);
void CreateBuffers(unsigned int& planeVAO, unsigned int& planeVBO, unsigned int& boxVAO, unsigned int& boxVBO);
void RenderImGui();
void RenderObjects(Shader& ourShader, unsigned int planeVAO, unsigned int boxVAO);
void Clear(unsigned int& planeVAO, unsigned int& boxVAO, unsigned int& planeVBO, unsigned int& boxVBO);

void PrepareNavmeshInput();
void UpdateDebugBuffers();
void RenderDebugNavmesh(Shader& debugShader);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f, 5.0f, 15.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::mat4 g_CubeModelMatrix = glm::mat4(1.0f);

// Main
int main()
{
    GLFWwindow* window;
    if (!CreateWindow(window))
        return -1;

    glEnable(GL_DEPTH_TEST);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    Shader ourShader("../source/shaders/simple.vert", "../source/shaders/simple.frag");
    unsigned int planeVAO, boxVAO, planeVBO, boxVBO;
    CreateBuffers(planeVAO, planeVBO, boxVAO, boxVBO);
    
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        RenderImGui();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        RenderObjects(ourShader, planeVAO, boxVAO);
        RenderDebugNavmesh(ourShader);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    Clear(planeVAO, boxVAO, planeVBO, boxVBO);
    return 0;
}

// Functions

bool CreateWindow(GLFWwindow*& window)
{
    if (!glfwInit())
    {
        std::cout << "GLFW initialization failed!" << std::endl;
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "NavMesh Example", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    return true;
}
void CreateBuffers(unsigned int& planeVAO, unsigned int& planeVBO, unsigned int& boxVAO, unsigned int& boxVBO)
{
    float planeVertices[] = {
        30.0f, 0.0f,  15.0f,
       -30.0f, 0.0f,  15.0f,
       -30.0f, 0.0f, -15.0f,

        30.0f, 0.0f,  15.0f,
       -30.0f, 0.0f, -15.0f,
        30.0f, 0.0f, -15.0f
   };
    float boxVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
       
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
    };

    // Ground VAO
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Box VAO
    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    glBindVertexArray(boxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), &boxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}
void RenderImGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    ImGui::Begin("Control Panel");
    if (ImGui::Button("Build NavMesh"))
    {
        PrepareNavmeshInput();
        UpdateDebugBuffers();
        std::cout << "NavMesh build triggered!" << std::endl;
    }
    ImGui::End();
}
void RenderObjects(Shader& ourShader, unsigned int planeVAO, unsigned int boxVAO)
{
    ourShader.use();

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    ourShader.setMat4("projection", projection);
    ourShader.setMat4("view", view);

    // Ground
    glm::mat4 model = glm::mat4(1.0f);
    ourShader.setMat4("model", model);
    ourShader.setVec4("ourColor", glm::vec4(0.4f, 0.4f, 0.4f, 1.0f)); // Gri zemin
    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Box
    g_CubeModelMatrix = glm::mat4(1.0f);
    g_CubeModelMatrix = glm::translate(g_CubeModelMatrix, glm::vec3(0.0f, 2.0f, 0.0f));
    g_CubeModelMatrix = glm::scale(g_CubeModelMatrix, glm::vec3(4.0f, 4.0f, 4.0f));
    ourShader.setMat4("model", g_CubeModelMatrix);
    ourShader.setVec4("ourColor", glm::vec4(0.8f, 0.2f, 0.2f, 1.0f)); // Kırmızı kutu
    glBindVertexArray(boxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}
void Clear(unsigned int& planeVAO, unsigned int& boxVAO, unsigned int& planeVBO, unsigned int& boxVBO)
{
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteVertexArrays(1, &boxVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteBuffers(1, &boxVBO);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwTerminate();
}

void PrepareNavmeshInput()
{
    float planeVerts[] = {
        30.0f, 0.0f,  15.0f, // 0
       -30.0f, 0.0f,  15.0f, // 1
       -30.0f, 0.0f, -15.0f, // 2
        30.0f, 0.0f, -15.0f  // 3
    };
    int planeTris[] = { 0, 1, 2,   0, 2, 3 };
    const int nPlaneTris = 2;

    float cubeVerts[] = {
        -0.5f, -0.5f, -0.5f, // 0
         0.5f, -0.5f, -0.5f, // 1
         0.5f,  0.5f, -0.5f, // 2
        -0.5f,  0.5f, -0.5f, // 3
        -0.5f, -0.5f,  0.5f, // 4
         0.5f, -0.5f,  0.5f, // 5
         0.5f,  0.5f,  0.5f, // 6
        -0.5f,  0.5f,  0.5f  // 7
    };
    int cubeTris[] = {
        0, 1, 2,   0, 2, 3, // Front
        4, 7, 6,   4, 6, 5, // Back
        0, 3, 7,   0, 7, 4, // Left
        1, 5, 6,   1, 6, 2, // Right
        3, 2, 6,   3, 6, 7, // Up
        0, 4, 5,   0, 5, 1  // Down
    };
    const int nCubeTris = 12;
    
    g_InputTriangles.clear();
    
    for (int i = 0; i < nPlaneTris; ++i)
    {
        const int* triIndices = &planeTris[i * 3];
        Triangle t;
        t.verts[0] = { planeVerts[triIndices[0] * 3], planeVerts[triIndices[0] * 3 + 1], planeVerts[triIndices[0] * 3 + 2] };
        t.verts[1] = { planeVerts[triIndices[1] * 3], planeVerts[triIndices[1] * 3 + 1], planeVerts[triIndices[1] * 3 + 2] };
        t.verts[2] = { planeVerts[triIndices[2] * 3], planeVerts[triIndices[2] * 3 + 1], planeVerts[triIndices[2] * 3 + 2] };
        g_InputTriangles.push_back(t);
    }
    
    for (int i = 0; i < nCubeTris; ++i)
    {
        const int* triIndices = &cubeTris[i * 3];
        
        Vec3f local_v1 = { cubeVerts[triIndices[0] * 3], cubeVerts[triIndices[0] * 3 + 1], cubeVerts[triIndices[0] * 3 + 2] };
        Vec3f local_v2 = { cubeVerts[triIndices[1] * 3], cubeVerts[triIndices[1] * 3 + 1], cubeVerts[triIndices[1] * 3 + 2] };
        Vec3f local_v3 = { cubeVerts[triIndices[2] * 3], cubeVerts[triIndices[2] * 3 + 1], cubeVerts[triIndices[2] * 3 + 2] };
        
        glm::vec4 world_v1_4 = g_CubeModelMatrix * glm::vec4(local_v1.x, local_v1.y, local_v1.z, 1.0f);
        glm::vec4 world_v2_4 = g_CubeModelMatrix * glm::vec4(local_v2.x, local_v2.y, local_v2.z, 1.0f);
        glm::vec4 world_v3_4 = g_CubeModelMatrix * glm::vec4(local_v3.x, local_v3.y, local_v3.z, 1.0f);
        
        Vec3f world_v1 = { world_v1_4.x, world_v1_4.y, world_v1_4.z };
        Vec3f world_v2 = { world_v2_4.x, world_v2_4.y, world_v2_4.z };
        Vec3f world_v3 = { world_v3_4.x, world_v3_4.y, world_v3_4.z };
        
        Triangle transformedTri;
        transformedTri.verts[0] = world_v1;
        transformedTri.verts[1] = world_v2;
        transformedTri.verts[2] = world_v3;
        
        g_InputTriangles.push_back(transformedTri);
    }

    std::cout << g_InputTriangles.size() << std::endl;
}

void UpdateDebugBuffers()
{
    if (g_InputTriangles.empty()) return;
    
    std::vector<float> debugVerts;
    debugVerts.reserve(g_InputTriangles.size() * 3 * 3);
    for (const auto& tri : g_InputTriangles)
    {
        for (int i = 0; i < 3; ++i)
        {
            debugVerts.push_back(tri.verts[i].x);
            debugVerts.push_back(tri.verts[i].y);
            debugVerts.push_back(tri.verts[i].z);
        }
    }
    
    if (g_debugVao == 0)
    {
        glGenVertexArrays(1, &g_debugVao);
        glGenBuffers(1, &g_debugVbo);
    }
    
    glBindVertexArray(g_debugVao);
    glBindBuffer(GL_ARRAY_BUFFER, g_debugVbo);
    glBufferData(GL_ARRAY_BUFFER, debugVerts.size() * sizeof(float), &debugVerts[0], GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void RenderDebugNavmesh(Shader& debugShader)
{
    if (g_debugVao == 0 || g_InputTriangles.empty()) return;
    
    debugShader.use();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    debugShader.setMat4("projection", projection);
    debugShader.setMat4("view", view);
    debugShader.setMat4("model", glm::mat4(1.0f));
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    debugShader.setVec4("ourColor", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); 
    
    glBindVertexArray(g_debugVao);
    glDrawArrays(GL_TRIANGLES, 0, g_InputTriangles.size() * 3);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(0);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);
        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; 
        lastX = xpos;
        lastY = ypos;
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}