#include "NavigationSystemDebugTools.h"
#include "NavSystem/NavigationSystem.h"

//Debug

NavigationSystemDebugTools::NavigationSystemDebugTools()
{

}

NavigationSystemDebugTools::~NavigationSystemDebugTools()
{
    glDeleteVertexArrays(1, &m_DebugVAO);
    glDeleteBuffers(1, &m_DebugVBO);
    m_DebugVAO = 0;
    m_DebugVBO = 0;
}

void NavigationSystemDebugTools::RenderDebugData(Camera& camera, Shader* debugShader, const Scene& scene, const std::vector<Triangle>& inputTriangles, const VoxelGrid& voxelGrid, HeightField& heightField,
    const ContourSet& contourSet, DebugDrawMode debugDrawMode)
{
    debugShader->use();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), 1280.0f/720.0f, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    debugShader->setMat4("projection", projection);
    debugShader->setMat4("view", view);

    DrawVoxelGridBounds(debugShader, scene, voxelGrid);

    switch (debugDrawMode)
    {
        case DRAWMODE_INPUT_TRIANGLES:
            DrawInputTriangles(debugShader, inputTriangles);
            break;
        case DRAWMODE_VOXELS:
            DrawVoxels_Solid(debugShader, scene, voxelGrid);
            break;
        case DRAWMODE_WALKABLE:
            DrawVoxels_Walkable(debugShader, scene, heightField);
            break;
        case DRAWMODE_REGIONS:
            DrawVoxels_Regions(debugShader, scene, heightField);
            break;
        case DRAWMODE_CONNECTIONS:
            DrawConnections(debugShader, heightField);
            break;
        case DRAWMODE_CONTOURS:
            DrawContours(debugShader, contourSet);
            break;
        case DRAWMODE_NONE:
            break;
    }
    
    glBindVertexArray(0);
}

void NavigationSystemDebugTools::DrawInputTriangles(Shader* shader, const std::vector<Triangle>& m_InputTriangles)
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

void NavigationSystemDebugTools::DrawVoxelGridBounds(Shader* shader, const Scene& scene, const VoxelGrid& m_VoxelGrid)
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

void NavigationSystemDebugTools::DrawVoxels_Solid(Shader* shader, const Scene& scene, const VoxelGrid& m_VoxelGrid)
{
    const MeshData* cubeMesh = scene.GetMesh("Cube");
    if (!cubeMesh || m_VoxelGrid.data.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(cubeMesh->VAO);

    for (int z = 0; z < m_VoxelGrid.depth; ++z)
    {
        for (int y = 0; y < m_VoxelGrid.height; ++y)
        {
            for (int x = 0; x < m_VoxelGrid.width; ++x)
            {
                int index = x + z * m_VoxelGrid.width + y * (m_VoxelGrid.width * m_VoxelGrid.depth);
                bool isSolid = m_VoxelGrid.data[index];

                glm::vec3 pos = {
                    m_VoxelGrid.minimumCorner.x + (x + 0.5f) * m_VoxelGrid.cellSize,
                    m_VoxelGrid.minimumCorner.y + (y + 0.5f) * m_VoxelGrid.cellHeight,
                    m_VoxelGrid.minimumCorner.z + (z + 0.5f) * m_VoxelGrid.cellSize
                };
                glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
                model = glm::scale(model, glm::vec3(m_VoxelGrid.cellSize, m_VoxelGrid.cellHeight, m_VoxelGrid.cellSize));
                shader->setMat4("model", model);

                if (isSolid) 
                    shader->setVec4("ourColor", glm::vec4(1.0f, 0.0f, 0.0f, 0.3f)); // Red for solid
                else 
                    shader->setVec4("ourColor", glm::vec4(0.0f, 1.0f, 0.0f, 0.03f)); // Green for empty
                
                glDrawElements(GL_TRIANGLES, cubeMesh->indices.size(), GL_UNSIGNED_INT, 0);
            }
        }
    }
    glDisable(GL_BLEND);
}

void NavigationSystemDebugTools::DrawVoxels_Walkable(Shader* shader, const Scene& scene, const HeightField& m_HeightField)
{
    if (m_HeightField.width == 0 || m_HeightField.spanPool.empty())
        return;

    const MeshData* cubeMesh = scene.GetMesh("Cube");
    if (!cubeMesh || cubeMesh->VAO == 0)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(cubeMesh->VAO);

    for (int z = 0; z < m_HeightField.depth; ++z)
    {
        for (int x = 0; x < m_HeightField.width; ++x)
        {
            for (HeightFieldSpan* span = m_HeightField.spans[x + z * m_HeightField.width]; span; span = span->next)
            {
                for (int y = span->spanMin; y <= span->spanMax; ++y)
                {
                    glm::vec3 pos =
                    {
                        m_HeightField.bmin.x + (x + 0.5f) * m_HeightField.cellSize,
                        m_HeightField.bmin.y + (y + 0.5f) * m_HeightField.cellHeight,
                        m_HeightField.bmin.z + (z + 0.5f) * m_HeightField.cellSize
                    };
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
                    model = glm::scale(model, glm::vec3(m_HeightField.cellSize, m_HeightField.cellHeight, m_HeightField.cellSize));
                    shader->setMat4("model", model);
                    
                    bool isWalkableSurface = (y == span->spanMax && span->areaID != 0);

                    if (isWalkableSurface)
                        shader->setVec4("ourColor", glm::vec4(0.0f, 0.5f, 1.0f, 0.7f));
                    else
                        shader->setVec4("ourColor", glm::vec4(1.0f, 0.0f, 0.0f, 0.3f));

                    glDrawElements(GL_TRIANGLES, cubeMesh->indices.size(), GL_UNSIGNED_INT, 0);
                }
            }
        }
    }
    glDisable(GL_BLEND);
}

void NavigationSystemDebugTools::DrawVoxels_Regions(Shader* shader, const Scene& scene, const HeightField& m_HeightField)
{
    if (m_HeightField.width == 0 || m_HeightField.spanPool.empty())
        return;

    const MeshData* cubeMesh = scene.GetMesh("Cube");
    if (!cubeMesh || cubeMesh->VAO == 0)
        return;
    
    const int numColors = 8;
    glm::vec4 regionColors[numColors] =
    {
        glm::vec4(0, 0, 1, 0.7f),
        glm::vec4(1, 1, 0, 0.7f),
        glm::vec4(0, 1, 1, 0.7f),
        glm::vec4(1, 0, 1, 0.7f),
        glm::vec4(1, 0.5f, 0, 0.7f),
        glm::vec4(0.5f, 0, 1, 0.7f),
        glm::vec4(1, 1, 1, 0.7f),
        glm::vec4(0.5f, 0.5f, 0.5f, 0.7f)
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(cubeMesh->VAO);

    const int w = m_HeightField.width;
    const int d = m_HeightField.depth;

    for (int z = 0; z < d; ++z)
    {
        for (int x = 0; x < w; ++x)
        {
            for (HeightFieldSpan* span = m_HeightField.spans[x + z * w]; span; span = span->next)
            {
                if (span->areaID != 0)
                {
                    int y = span->spanMax;
                    glm::vec3 pos =
                    {
                        m_HeightField.bmin.x + (x + 0.5f) * m_HeightField.cellSize,
                        m_HeightField.bmin.y + (y + 0.5f) * m_HeightField.cellHeight,
                        m_HeightField.bmin.z + (z + 0.5f) * m_HeightField.cellSize
                    };
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
                    model = glm::scale(model, glm::vec3(m_HeightField.cellSize, m_HeightField.cellHeight, m_HeightField.cellSize));
                    shader->setMat4("model", model);
                    
                    unsigned int areaId = span->areaID;
                    if (areaId > 1)
                    { 
                        int colorIndex = (areaId - 2) % numColors;
                        shader->setVec4("ourColor", regionColors[colorIndex]);
                    }
                    else
                        shader->setVec4("ourColor", glm::vec4(1.0f, 0.0f, 0.0f, 0.5f));
                    
                    glDrawElements(GL_TRIANGLES, cubeMesh->indices.size(), GL_UNSIGNED_INT, 0);
                }
            }
        }
    }
    glDisable(GL_BLEND);
}

void NavigationSystemDebugTools::DrawConnections(Shader* shader, HeightField& m_HeightField)
{
    if (m_HeightField.width == 0 || m_HeightField.spanPool.empty())
        return;
    
    std::vector<float> lineVerts;

    const int w = m_HeightField.width;
    const int d = m_HeightField.depth;
    
    for (int z = 0; z < d; ++z)
    {
        for (int x = 0; x < w; ++x)
        {
            for (HeightFieldSpan* span = m_HeightField.spans[x + z * w]; span; span = span->next)
            {
                if (span->areaID == 0)
                    continue;
                
                glm::vec3 p0 =
                {
                    m_HeightField.bmin.x + (x + 0.5f) * m_HeightField.cellSize,
                    m_HeightField.bmin.y + (span->spanMax + 1) * m_HeightField.cellHeight,
                    m_HeightField.bmin.z + (z + 0.5f) * m_HeightField.cellSize
                };
                
                for (int dir = 0; dir < 4; ++dir)
                {
                    unsigned int connIndex = span->connections[dir];
                    if (connIndex == 0)
                        continue;
                    
                    HeightFieldSpan* neighborSpan = &m_HeightField.spanPool[connIndex - 1];
                    int dx[] = {-1, 0, 1, 0};
                    int dz[] = {0, -1, 0, 1};
                    int nx = x + dx[dir];
                    int nz = z + dz[dir];
                    
                    glm::vec3 p1 =
                    {
                        m_HeightField.bmin.x + (nx + 0.5f) * m_HeightField.cellSize,
                        m_HeightField.bmin.y + (neighborSpan->spanMax + 1) * m_HeightField.cellHeight,
                        m_HeightField.bmin.z + (nz + 0.5f) * m_HeightField.cellSize
                    };
                    
                    lineVerts.push_back(p0.x); lineVerts.push_back(p0.y); lineVerts.push_back(p0.z);
                    lineVerts.push_back(p1.x); lineVerts.push_back(p1.y); lineVerts.push_back(p1.z);
                }
            }
        }
    }
    
    if (lineVerts.empty()) return;
    
    if (m_ConnectionLinesVAO == 0)
    {
        glGenVertexArrays(1, &m_ConnectionLinesVAO);
        glGenBuffers(1, &m_ConnectionLinesVBO);
    }
    
    glBindVertexArray(m_ConnectionLinesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_ConnectionLinesVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVerts.size() * sizeof(float), lineVerts.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    shader->setMat4("model", glm::mat4(1.0f));
    shader->setVec4("ourColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, lineVerts.size() / 3);
    glLineWidth(1.0f);
}

void NavigationSystemDebugTools::DrawContours(Shader* shader, const ContourSet& contourSet)
{
    if (contourSet.contours.empty()) return;

    std::vector<float> lineVerts;
    const float cs = contourSet.cellSize;
    const float ch = contourSet.cellHeight;
    const glm::vec3& bmin = contourSet.bmin;

    for (const auto& contour : contourSet.contours) {
        if (contour.vertices.empty()) continue;

        for (size_t i = 0; i < contour.vertices.size() / 4; ++i) {
            const int* v0_data = &contour.vertices[i * 4];
            // Connect to the next vertex in the list, wrapping around at the end
            const int* v1_data = &contour.vertices[((i + 1) % (contour.vertices.size() / 4)) * 4];

            glm::vec3 p0 = { bmin.x + v0_data[0] * cs, bmin.y + (v0_data[1] + 1) * ch + 0.1f, bmin.z + v0_data[2] * cs };
            glm::vec3 p1 = { bmin.x + v1_data[0] * cs, bmin.y + (v1_data[1] + 1) * ch + 0.1f, bmin.z + v1_data[2] * cs };
            
            lineVerts.push_back(p0.x); lineVerts.push_back(p0.y); lineVerts.push_back(p0.z);
            lineVerts.push_back(p1.x); lineVerts.push_back(p1.y); lineVerts.push_back(p1.z);
        }
    }

    if (lineVerts.empty()) return;

    if (m_ContourLinesVAO == 0) {
        glGenVertexArrays(1, &m_ContourLinesVAO);
        glGenBuffers(1, &m_ContourLinesVBO);
    }
    
    glBindVertexArray(m_ContourLinesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_ContourLinesVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVerts.size() * sizeof(float), lineVerts.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    shader->setMat4("model", glm::mat4(1.0f));
    shader->setVec4("ourColor", glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)); // Magenta for high visibility

    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, lineVerts.size() / 3);
    glLineWidth(1.0f);

    glBindVertexArray(0);
}

void NavigationSystemDebugTools::UpdateDebugBuffers(const std::vector<Triangle>& m_InputTriangles)
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
