#include "Scene.h"

#include <iostream>
#include <glm/ext/matrix_transform.hpp>

Scene::Scene() : m_Meshes(), m_Objects()
{
    CreateDefaultMeshes();
}

Scene::~Scene()
{
    for (auto& pair : m_Meshes)
    {
        glDeleteVertexArrays(1, &pair.second.VAO);
        glDeleteBuffers(1, &pair.second.VBO);
        glDeleteBuffers(1, &pair.second.EBO);
    }
}

void Scene::SetupDefaultScene()
{
    m_Objects.clear();

    // Ground
    glm::mat4 groundTransform = glm::mat4(1.0f);
    groundTransform = glm::translate(groundTransform, glm::vec3(0.0f, -0.05f, 0.0f));
    groundTransform = glm::scale(groundTransform, glm::vec3(30.0f, 0.1f, 30.0f));
    AddObject("Ground", "Cube", groundTransform);

    // Box
    glm::mat4 obstacleTransform = glm::mat4(1.0f);
    obstacleTransform = glm::translate(obstacleTransform, glm::vec3(0.0f, 2.0f, 0.0f));
    obstacleTransform = glm::scale(obstacleTransform, glm::vec3(4.0f, 4.0f, 4.0f));
    AddObject("Obstacle", "Cube", obstacleTransform);
}

void Scene::Render(Shader* shader)
{
    if (!shader || m_Objects.empty())
        return;

    shader->use();
    
    for (const auto& object : m_Objects)
    {
        if (!object.mesh)
            continue;
        
        shader->setMat4("model", object.modelMatrix);
        
        if (object.name == "Ground")
            shader->setVec4("ourColor", glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
        else
            shader->setVec4("ourColor", glm::vec4(0.8f, 0.2f, 0.2f, 1.0f));
        
        glBindVertexArray(object.mesh->VAO);
        glDrawElements(GL_TRIANGLES, object.mesh->indices.size(), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

SceneObject* Scene::AddObject(const std::string& name, const std::string& meshName, const glm::mat4& modelMatrix)
{
    if (m_Meshes.find(meshName) == m_Meshes.end())
    {
        std::cout << "Mesh " << meshName << " not found!" << std::endl;
        return nullptr;
    }
    SceneObject newObj;
    newObj.name = name;
    newObj.mesh = &m_Meshes.at(meshName);
    newObj.modelMatrix = modelMatrix;
    m_Objects.push_back(newObj);
    return &m_Objects.back();
}

void Scene::CreateDefaultMeshes()
{
    MeshData cubeMesh;
    cubeMesh.vertices = {
        {-0.5f, -0.5f, -0.5f}, // 0
        { 0.5f, -0.5f, -0.5f}, // 1
        { 0.5f,  0.5f, -0.5f}, // 2
        {-0.5f,  0.5f, -0.5f}, // 3
        {-0.5f, -0.5f,  0.5f}, // 4
        { 0.5f, -0.5f,  0.5f}, // 5
        { 0.5f,  0.5f,  0.5f}, // 6
        {-0.5f,  0.5f,  0.5f}  // 7
    };
    cubeMesh.indices = {
        0, 1, 2,   0, 2, 3, // Front
        4, 7, 6,   4, 6, 5, // Back
        0, 3, 7,   0, 7, 4, // Left
        1, 5, 6,   1, 6, 2, // Right
        3, 2, 6,   3, 6, 7, // Up
        0, 4, 5,   0, 5, 1  // Down
    };
    m_Meshes["Cube"] = cubeMesh;

    for (auto& [key, mesh] : m_Meshes) {
        glGenVertexArrays(1, &mesh.VAO);
        glGenBuffers(1, &mesh.VBO);
        glGenBuffers(1, &mesh.EBO);

        glBindVertexArray(mesh.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vec3f), mesh.vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3f), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
}
