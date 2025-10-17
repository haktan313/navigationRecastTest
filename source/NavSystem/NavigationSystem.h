#pragma once

#include "Core/Scene.h"


struct NavMesh
{
    
};
struct VoxelGrid
{
    glm::vec3 minimumCorner;
    glm::vec3 maximumCorner;
    float cellSize, cellHeight;
    int width, depth, height;
    std::vector<bool> data; // true = walkable, false = not walkable
};

struct HeightFieldSpan
{
    unsigned int spanMin, spanMax;
    unsigned int areaID;
    HeightFieldSpan* next;
};
struct HeightField
{
    int width, depth;
    glm::vec3 bmin;
    float cellSize, cellHeight;

    HeightFieldSpan** spans;
    std::vector<HeightFieldSpan> spanPool;
};

class NavigationSystem
{
public:
    NavigationSystem();
    ~NavigationSystem();
    
    void BuildNavMesh(const Scene& scene);
    
    void RenderDebugNavmesh(Shader* debugShader, const Scene& scene);
private:
    void DrawTriangle(Shader* shader, const Scene& scene);
    void DrawVoxel(Shader* shader, const Scene& scene);
    void DrawVoxelGrids(Shader* shader, const Scene& scene);
    unsigned int m_DebugVAO, m_DebugVBO;
    void UpdateDebugBuffers();
    
    std::vector<Triangle> m_InputTriangles;
    NavMesh m_NavMesh;
    
    void Voxelize();
    VoxelGrid m_VoxelGrid;
    void Rasterization();
    void BuildHeightField();
    HeightField m_HeightField;
    void FilterWalkableSurfaces();
    float m_AgentHeight, m_AgentRadius, m_MaxClimb;


    
    bool TriBoxOverlap(const float boxcenter[3], const float boxhalfsize[3], const float triverts[3][3]);

};
