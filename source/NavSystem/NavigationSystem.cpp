#include "NavigationSystem.h"

NavigationSystem::NavigationSystem() : m_InputTriangles(), m_NavMesh(), m_DebugVAO(0), m_DebugVBO(0)
{
    std::cout << "NavigationSystem initialized." << std::endl;
}

NavigationSystem::~NavigationSystem()
{
    glDeleteVertexArrays(1, &m_DebugVAO);
    glDeleteBuffers(1, &m_DebugVBO);
    m_DebugVAO = 0;
    m_DebugVBO = 0;
}

void NavigationSystem::BuildNavMesh(const Scene& scene)
{
    std::cout << "Building NavMesh from scene..." << std::endl;
    m_InputTriangles.clear();
    const auto& objects = scene.GetObjects();

    for (const auto& obj : objects)
    {
        if (!obj.mesh)
            continue;
        const glm::mat4& modelMatrix = obj.modelMatrix;
        const MeshData* mesh = obj.mesh;

        for (size_t i = 0; i < mesh->indices.size() / 3; ++i)
        {
            unsigned int idx0 = mesh->indices[i * 3];
            unsigned int idx1 = mesh->indices[i * 3 + 1];
            unsigned int idx2 = mesh->indices[i * 3 + 2];

            const Vec3f& local_v0 = mesh->vertices[idx0];
            const Vec3f& local_v1 = mesh->vertices[idx1];
            const Vec3f& local_v2 = mesh->vertices[idx2];

            glm::vec4 world_v0 = modelMatrix * glm::vec4(local_v0.x, local_v0.y, local_v0.z, 1.0f);
            glm::vec4 world_v1 = modelMatrix * glm::vec4(local_v1.x, local_v1.y, local_v1.z, 1.0f);
            glm::vec4 world_v2 = modelMatrix * glm::vec4(local_v2.x, local_v2.y, local_v2.z, 1.0f);

            Triangle tri;
            tri.verts[0] = { world_v0.x, world_v0.y, world_v0.z };
            tri.verts[1] = { world_v1.x, world_v1.y, world_v1.z };
            tri.verts[2] = { world_v2.x, world_v2.y, world_v2.z };
            m_InputTriangles.push_back(tri);
        }
    }
    std::cout << "Collected " << m_InputTriangles.size() << " triangles for NavMesh." << std::endl;
    UpdateDebugBuffers();
}

void NavigationSystem::RenderDebugNavmesh(Shader* debugShader)
{
    if (m_DebugVAO == 0 || m_InputTriangles.empty())
        return;
    
    debugShader->setMat4("model", glm::mat4(1.0f));
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    debugShader->setVec4("ourColor", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); 
    
    glBindVertexArray(m_DebugVAO);
    glDrawArrays(GL_TRIANGLES, 0, m_InputTriangles.size() * 3);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindVertexArray(0);
}

void NavigationSystem::UpdateDebugBuffers()
{
    if (m_InputTriangles.empty())
        return;

    std::vector<float> debugVerts;
    debugVerts.reserve(m_InputTriangles.size() * 3 * 3);
    for (const auto& tri : m_InputTriangles)
        for (int i = 0; i < 3; ++i)
        {
            debugVerts.push_back(tri.verts[i].x);
            debugVerts.push_back(tri.verts[i].y);
            debugVerts.push_back(tri.verts[i].z);
        }

    if (m_DebugVAO == 0)
    {
        glGenVertexArrays(1, &m_DebugVAO);
        glGenBuffers(1, &m_DebugVBO);
    }
    
    glBindVertexArray(m_DebugVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_DebugVBO);
    glBufferData(GL_ARRAY_BUFFER, debugVerts.size() * sizeof(float), &debugVerts[0], GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}