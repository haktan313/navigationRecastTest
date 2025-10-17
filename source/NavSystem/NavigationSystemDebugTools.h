#pragma once
#include "Core/Scene.h"
#include "Core/Camera.h"
#include "Core/Shader.h"
#include <vector>

struct Triangle;
struct VoxelGrid;
struct HeightField;
enum DebugDrawMode;

class NavigationSystemDebugTools
{
public:
    NavigationSystemDebugTools();
    ~NavigationSystemDebugTools();
    
    void RenderDebugData(Camera& camera, Shader* debugShader, const Scene& scene, const std::vector<Triangle>& inputTriangles, const VoxelGrid& voxelGrid, const HeightField& heightField, DebugDrawMode debugDrawMode);
private:
    unsigned int m_DebugVAO, m_DebugVBO;
    void DrawInputTriangles(Shader* shader, const std::vector<Triangle>& m_InputTriangles);
    void DrawVoxelGridBounds(Shader* shader, const Scene& scene, const VoxelGrid& m_VoxelGrid);
    void DrawVoxels_Solid(Shader* shader, const Scene& scene, const VoxelGrid& m_VoxelGrid);
    void DrawVoxels_Walkable(Shader* shader, const Scene& scene, const HeightField& m_HeightField);
    void DrawVoxels_Regions(Shader* shader, const Scene& scene, const HeightField& m_HeightField);
public:
    void UpdateDebugBuffers(const std::vector<Triangle>& m_InputTriangles);
};
