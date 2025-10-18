#pragma once
#include "Core/Scene.h"
#include "Core/Camera.h"
#include "Core/Shader.h"
#include <vector>

struct Triangle;
struct VoxelGrid;
struct HeightField;
struct ContourSet;
enum DebugDrawMode;

class NavigationSystemDebugTools
{
public:
    NavigationSystemDebugTools();
    ~NavigationSystemDebugTools();
    
    void RenderDebugData(Camera& camera, Shader* debugShader, const Scene& scene, const std::vector<Triangle>& inputTriangles, const VoxelGrid&
                         voxelGrid, HeightField& heightField, const ContourSet& contourSet, DebugDrawMode
                         debugDrawMode);
private:
    unsigned int m_DebugVAO = 0, m_DebugVBO = 0;
    unsigned int m_ConnectionLinesVAO = 0, m_ConnectionLinesVBO = 0;
    unsigned int m_ContourLinesVAO = 0, m_ContourLinesVBO = 0;
    void DrawInputTriangles(Shader* shader, const std::vector<Triangle>& m_InputTriangles);
    void DrawVoxelGridBounds(Shader* shader, const Scene& scene, const VoxelGrid& m_VoxelGrid);
    void DrawVoxels_Solid(Shader* shader, const Scene& scene, const VoxelGrid& m_VoxelGrid);
    void DrawVoxels_Walkable(Shader* shader, const Scene& scene, const HeightField& m_HeightField);
    void DrawVoxels_Regions(Shader* shader, const Scene& scene, const HeightField& m_HeightField);
    void DrawConnections(Shader* shader, HeightField& m_HeightField);
    void DrawContours(Shader* shader, const ContourSet& contourSet);
public:
    void UpdateDebugBuffers(const std::vector<Triangle>& m_InputTriangles);
};
