
#include "NavigationSystem.h"
#include <glm/ext/matrix_transform.hpp>

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
    Voxelize();
    UpdateDebugBuffers();
}

void NavigationSystem::RenderDebugNavmesh(Shader* debugShader, const Scene& scene)
{
    DrawTriangle(debugShader, scene);
    DrawVoxels(debugShader, scene);
    DrawVoxelGridBounds(debugShader, scene);
    
    glBindVertexArray(0);
}

void NavigationSystem::DrawTriangle(Shader* shader, const Scene& scene)
{
    if (m_DebugVAO != 0 && !m_InputTriangles.empty())
    {
        shader->setMat4("model", glm::mat4(1.0f));
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        shader->setVec4("ourColor", glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
        
        glBindVertexArray(m_DebugVAO);
        glDrawArrays(GL_TRIANGLES, 0, m_InputTriangles.size() * 3);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void NavigationSystem::DrawVoxels(Shader* shader, const Scene& scene)
{
    if (m_VoxelGrid.width > 0)
    {
        const MeshData* cubeMesh = scene.GetMesh("Cube");
        if (cubeMesh)
        {
            glm::vec3 size = m_VoxelGrid.maximumCorner - m_VoxelGrid.minimumCorner;
            glm::vec3 center = (m_VoxelGrid.minimumCorner + m_VoxelGrid.maximumCorner) * 0.5f;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, center);
            model = glm::scale(model, size);
            shader->setMat4("model", model);
            shader->setVec4("ourColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.5f));
            
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            
            glBindVertexArray(cubeMesh->VAO);
            glDrawElements(GL_TRIANGLES, cubeMesh->indices.size(), GL_UNSIGNED_INT, 0);
            
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_BLEND);
        }
    }
}

void NavigationSystem::DrawVoxelGridBounds(Shader* shader, const Scene& scene)
{
    if (m_VoxelGrid.width > 0 && !m_VoxelGrid.data.empty())
    {
        const MeshData* cubeMesh = scene.GetMesh("Cube");
        if (cubeMesh)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBindVertexArray(cubeMesh->VAO);
            
            for (int z = 0; z < m_VoxelGrid.depth; ++z)
            {
                for (int y = 0; y < m_VoxelGrid.height; ++y)
                {
                    for (int x = 0; x < m_VoxelGrid.width; ++x)
                    {
                        int index = x + z * m_VoxelGrid.width + y * m_VoxelGrid.width * m_VoxelGrid.depth;
                        bool isSolid = m_VoxelGrid.data[index];
                        
                        glm::vec3 pos = {
                            m_VoxelGrid.minimumCorner.x + (x + 0.5f) * m_VoxelGrid.cellSize,
                            m_VoxelGrid.minimumCorner.y + (y + 0.5f) * m_VoxelGrid.cellHeight,
                            m_VoxelGrid.minimumCorner.z + (z + 0.5f) * m_VoxelGrid.cellSize
                        };
                        glm::mat4 model = glm::mat4(1.0f);
                        model = glm::translate(model, pos);
                        model = glm::scale(model, glm::vec3(m_VoxelGrid.cellSize, m_VoxelGrid.cellHeight, m_VoxelGrid.cellSize));
                        
                        shader->setMat4("model", model);
                        
                        if (isSolid) 
                            shader->setVec4("ourColor", glm::vec4(1.0f, 0.0f, 0.0f, 0.1f)); // Red for solid
                        else 
                            shader->setVec4("ourColor", glm::vec4(0.0f, 1.0f, 0.0f, 0.1f)); // Green for empty
                        
                        glDrawElements(GL_TRIANGLES, cubeMesh->indices.size(), GL_UNSIGNED_INT, 0);
                    }
                }
            }
            glDisable(GL_BLEND);
        }
    }
}

void NavigationSystem::Voxelize()
{
    std::cout << "Voxelization step (placeholder)..." << std::endl;
    if (m_InputTriangles.empty())
    {
        std::cout << "No input triangles to voxelize." << std::endl;
        return;
    }

    m_VoxelGrid.minimumCorner = glm::vec3(-15.0f, -1.0f, -15.0f);
    m_VoxelGrid.maximumCorner = glm::vec3(15.0f, 10.0f, 15.0f);
    m_VoxelGrid.cellSize = 5.f;
    m_VoxelGrid.cellHeight = 5.f;

    if (m_VoxelGrid.minimumCorner.x >= m_VoxelGrid.maximumCorner.x ||
        m_VoxelGrid.minimumCorner.y >= m_VoxelGrid.maximumCorner.y ||
        m_VoxelGrid.minimumCorner.z >= m_VoxelGrid.maximumCorner.z ||
        m_VoxelGrid.cellSize <= 0.0f || m_VoxelGrid.cellHeight <= 0.0f)
    {
        std::cout << "Invalid voxel grid parameters." << std::endl;
        return;
    }

    m_VoxelGrid.width = (int)((m_VoxelGrid.maximumCorner.x - m_VoxelGrid.minimumCorner.x) / m_VoxelGrid.cellSize);
    m_VoxelGrid.depth = (int)((m_VoxelGrid.maximumCorner.z - m_VoxelGrid.minimumCorner.z) / m_VoxelGrid.cellSize);
    m_VoxelGrid.height = (int)((m_VoxelGrid.maximumCorner.y - m_VoxelGrid.minimumCorner.y) / m_VoxelGrid.cellHeight);
    int totalVoxels = m_VoxelGrid.width * m_VoxelGrid.depth * m_VoxelGrid.height;
    std::cout << "Voxel Grid Dimensions: " << m_VoxelGrid.width << " x " << m_VoxelGrid.depth << " x " << m_VoxelGrid.height << " = " << totalVoxels << " voxels." << std::endl;

    m_VoxelGrid.data.assign(totalVoxels, false);
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
