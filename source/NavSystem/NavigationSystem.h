#pragma once
#include "Core/Scene.h"


struct NavMesh
{
    
};

class NavigationSystem
{
public:
    NavigationSystem();
    ~NavigationSystem();

    void BuildNavMesh(const Scene& scene);
    void RenderDebugNavmesh(Shader* debugShader);
private:

    std::vector<Triangle> m_InputTriangles;
    NavMesh m_NavMesh;

    unsigned int m_DebugVAO, m_DebugVBO;
    void UpdateDebugBuffers();
};
