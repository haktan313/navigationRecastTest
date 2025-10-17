#pragma once

#include "NavigationSystemDebugTools.h"
#include "Core/Camera.h"
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
enum DebugDrawMode
{
    DRAWMODE_NONE,
    DRAWMODE_INPUT_TRIANGLES,
    DRAWMODE_VOXELS,
    DRAWMODE_WALKABLE,
    DRAWMODE_REGIONS
};


class NavigationSystem
{
public:
    DebugDrawMode m_DebugDrawMode = DRAWMODE_NONE;
    
    NavigationSystem();
    ~NavigationSystem();
    
    void BuildNavMesh(const Scene& scene);
    
    void RenderDebugData(Camera& camera, Shader* debugShader, const Scene& scene);
private:
    NavigationSystemDebugTools* m_DebugTools;

    std::vector<Triangle> m_InputTriangles;
    float m_AgentHeight, m_AgentRadius, m_MaxClimb;

    NavMesh m_NavMesh;
    VoxelGrid m_VoxelGrid;
    HeightField m_HeightField;
    
    void Voxelize();
    void Rasterization();
    void BuildHeightField();
    void FilterWalkableSurfaces();
    void BuldRegions();
    
    bool TriBoxOverlap(const float boxcenter[3], const float boxhalfsize[3], const float triverts[3][3]);
};
