
#include "NavigationSystem.h"
#include <deque>

NavigationSystem::NavigationSystem() : m_InputTriangles(), m_NavMesh()
{
    std::cout << "NavigationSystem initialized." << std::endl;
    m_AgentHeight = 2.0f;
    m_AgentRadius = 0.6f;
    m_MaxClimb = 0.9f;
    m_DebugTools = new NavigationSystemDebugTools();
}

NavigationSystem::~NavigationSystem()
{
    delete m_DebugTools;
    m_DebugTools = nullptr;
    if (m_HeightField.spans)
    {
        delete[] m_HeightField.spans;
        m_HeightField.spans = nullptr;
    }
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
    BuildHeightField();
    FilterWalkableSurfaces();
    BuldRegions();

    if (m_DebugTools)
        m_DebugTools->UpdateDebugBuffers(m_InputTriangles);
}

void NavigationSystem::RenderDebugData(Camera& camera, Shader* debugShader, const Scene& scene)
{
    if (m_DebugTools)
        m_DebugTools->RenderDebugData(camera, debugShader, scene, m_InputTriangles, m_VoxelGrid, m_HeightField, m_DebugDrawMode);
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
    m_VoxelGrid.cellSize = 1.f;
    m_VoxelGrid.cellHeight = 1.f;

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

    Rasterization();
}

void NavigationSystem::Rasterization()
{
    int solidVoxels = 0;
    for (const auto& tri : m_InputTriangles)
    {
        float triMin[3] = { tri.verts[0].x, tri.verts[0].y, tri.verts[0].z };
        float triMax[3] = { tri.verts[0].x, tri.verts[0].y, tri.verts[0].z };
        for (int i = 1; i < 3; ++i)
        {
            triMin[0] = std::min(triMin[0], tri.verts[i].x);
            triMin[1] = std::min(triMin[1], tri.verts[i].y);
            triMin[2] = std::min(triMin[2], tri.verts[i].z);
            
            triMax[0] = std::max(triMax[0], tri.verts[i].x);
            triMax[1] = std::max(triMax[1], tri.verts[i].y);
            triMax[2] = std::max(triMax[2], tri.verts[i].z);
        }

        int minX = (int)((triMin[0] - m_VoxelGrid.minimumCorner.x) / m_VoxelGrid.cellSize);
        int minY = (int)((triMin[1] - m_VoxelGrid.minimumCorner.y) / m_VoxelGrid.cellHeight);
        int minZ = (int)((triMin[2] - m_VoxelGrid.minimumCorner.z) / m_VoxelGrid.cellSize);
        
        int maxX = (int)((triMax[0] - m_VoxelGrid.minimumCorner.x) / m_VoxelGrid.cellSize);
        int maxY = (int)((triMax[1] - m_VoxelGrid.minimumCorner.y) / m_VoxelGrid.cellHeight);
        int maxZ = (int)((triMax[2] - m_VoxelGrid.minimumCorner.z) / m_VoxelGrid.cellSize);

        minX = std::max(0, minX);
        minY = std::max(0, minY);
        minZ = std::max(0, minZ);

        maxX = std::min(m_VoxelGrid.width - 1, maxX);
        maxY = std::min(m_VoxelGrid.height - 1, maxY);
        maxZ = std::min(m_VoxelGrid.depth - 1, maxZ);

        for (int z = minZ; z <= maxZ; ++z)
        {
            for (int y = minY; y <= maxY; ++y)
            {
                for (int x = minX; x <= maxX; ++x)
                {
                    int index = x + z * m_VoxelGrid.width + y * m_VoxelGrid.width * m_VoxelGrid.depth;
                    if (m_VoxelGrid.data[index])
                        continue;

                    float boxcenter[3] = {
                        m_VoxelGrid.minimumCorner.x + (x + 0.5f) * m_VoxelGrid.cellSize,
                        m_VoxelGrid.minimumCorner.y + (y + 0.5f) * m_VoxelGrid.cellHeight,
                        m_VoxelGrid.minimumCorner.z + (z + 0.5f) * m_VoxelGrid.cellSize
                    };
                    float boxhalfsize[3] = {
                        m_VoxelGrid.cellSize * 0.5f,
                        m_VoxelGrid.cellHeight * 0.5f,
                        m_VoxelGrid.cellSize * 0.5f
                    };
                    float triverts[3][3] = {
                        { tri.verts[0].x, tri.verts[0].y, tri.verts[0].z },
                        { tri.verts[1].x, tri.verts[1].y, tri.verts[1].z },
                        { tri.verts[2].x, tri.verts[2].y, tri.verts[2].z }
                    };

                    if (TriBoxOverlap(boxcenter, boxhalfsize, triverts))
                    {
                        m_VoxelGrid.data[index] = true;
                        solidVoxels++;
                    }
                }
            }
        }
    }
    std::cout << "Rasterization complete. Solid voxels: " << solidVoxels << std::endl;
}

void NavigationSystem::BuildHeightField()
{
    m_HeightField.width = m_VoxelGrid.width;
    m_HeightField.depth = m_VoxelGrid.depth;
    m_HeightField.cellSize = m_VoxelGrid.cellSize;
    m_HeightField.cellHeight = m_VoxelGrid.cellHeight;
    m_HeightField.bmin = m_VoxelGrid.minimumCorner;

    const int numColumns = m_HeightField.width * m_HeightField.depth;
    m_HeightField.spans = new HeightFieldSpan*[numColumns];
    memset(m_HeightField.spans, 0, sizeof(HeightFieldSpan*) * numColumns);

    m_HeightField.spanPool.clear();
    m_HeightField.spanPool.reserve(m_VoxelGrid.width * m_VoxelGrid.depth * m_VoxelGrid.height);

    for (int z = 0; z < m_HeightField.depth; ++z)
    {
        for (int x = 0; x < m_HeightField.width; ++x)
        {
            HeightFieldSpan* previousSpan = nullptr;
            for (int y = 0; y < m_VoxelGrid.height; ++y)
            {
                int index = x + z * m_VoxelGrid.width + y * m_VoxelGrid.width * m_VoxelGrid.depth;
                bool bisSolidCurrent = m_VoxelGrid.data[index];

                int previousIndex = index - m_VoxelGrid.width * m_VoxelGrid.depth;
                bool bisSolidPrevious = (y > 0) ? m_VoxelGrid.data[previousIndex] : false;

                if (bisSolidCurrent != bisSolidPrevious)
                {
                    if (bisSolidCurrent)
                    {
                        HeightFieldSpan newSpan;
                        newSpan.spanMin = y;
                        newSpan.spanMax = y;
                        newSpan.areaID = 0;
                        newSpan.next = nullptr;

                        m_HeightField.spanPool.push_back(newSpan);
                        HeightFieldSpan* currentSpan = &m_HeightField.spanPool.back();

                        if (previousSpan)
                            previousSpan->next = currentSpan;
                        else
                            m_HeightField.spans[x + z * m_HeightField.width] = currentSpan;
                        previousSpan = currentSpan;
                    }
                }
                if (bisSolidCurrent && previousSpan)
                {
                    previousSpan->spanMax = y;
                }
            }
        }
    }
    std::cout << "Heightfield built with " << m_HeightField.spanPool.size() << " spans." << std::endl;
}

void NavigationSystem::FilterWalkableSurfaces()
{
    const int walkableHeight = (int)ceilf(m_AgentHeight / m_HeightField.cellHeight);
    
    for (auto& span : m_HeightField.spanPool)
        span.areaID = 1;
    
    for (int z = 0; z < m_HeightField.depth; ++z)
    {
        for (int x = 0; x < m_HeightField.width; ++x)
        {
            for (HeightFieldSpan* span = m_HeightField.spans[x + z * m_HeightField.width]; span; span = span->next)
            {
                const int upperSpanFloor = span->next ? (int)span->next->spanMin : m_VoxelGrid.height;
                const int headroom = upperSpanFloor - (int)span->spanMax;
                
                if (headroom < walkableHeight)
                {
                    span->areaID = 0;
                }
            }
        }
    }
    std::cout << "Walkable surfaces filtered." << std::endl;
}

void NavigationSystem::BuldRegions()
{
    std::cout << "Building regions..." << std::endl;
    if (m_HeightField.width == 0 || m_HeightField.depth == 0)
        return;

    const int walkableClimb = m_MaxClimb > 0 ? (int)floorf(m_MaxClimb / m_HeightField.cellHeight) : 0;
    
    unsigned int regionId = 2;
    
    struct SpanLocation {
        int x, z;
        HeightFieldSpan* span;
    };
    
    for (int z = 0; z < m_HeightField.depth; ++z)
    {
        for (int x = 0; x < m_HeightField.width; ++x)
        {
            for (HeightFieldSpan* span = m_HeightField.spans[x + z * m_HeightField.width]; span; span = span->next)
            {
                if (span->areaID == 1)
                {
                    std::deque<SpanLocation> openList;
                    openList.push_back({x, z, span});
                    span->areaID = regionId;

                    while (!openList.empty())
                    {
                        SpanLocation current = openList.front();
                        openList.pop_front();
                        
                        for (int dir = 0; dir < 4; ++dir)
                        {
                            int dx[] = {-1, 0, 1, 0};
                            int dz[] = {0, -1, 0, 1};
                            int nx = current.x + dx[dir];
                            int nz = current.z + dz[dir];
                            
                            if (nx < 0 || nz < 0 || nx >= m_HeightField.width || nz >= m_HeightField.depth)
                                continue;
                            
                            for (HeightFieldSpan* neighborSpan = m_HeightField.spans[nx + nz * m_HeightField.width]; neighborSpan; neighborSpan = neighborSpan->next)
                            {
                                if (neighborSpan->areaID == 1)
                                {
                                    const int heightDiff = abs((int)current.span->spanMax - (int)neighborSpan->spanMax);
                                    if (heightDiff <= walkableClimb)
                                    {
                                        neighborSpan->areaID = regionId;
                                        openList.push_back({nx, nz, neighborSpan});
                                    }
                                }
                            }
                        }
                    }
                    regionId++;
                }
            }
        }
    }

    std::cout << "Regions built. Total regions found: " << regionId - 2 << std::endl;
}

// --- Triangle-Box Overlap Test (by Tomas Akenine-Möller) ---

#define X 0
#define Y 1
#define Z 2

#define FINDMINMAX(x0, x1, x2, min, max) \
  min = max = x0;                       \
  if(x1<min) min=x1;                    \
  if(x1>max) max=x1;                    \
  if(x2<min) min=x2;                    \
  if(x2>max) max=x2;

#define AXISTEST_X01(a, b, fa, fb)                 \
    p0 = a*v0[Y] - b*v0[Z];                        \
    p2 = a*v2[Y] - b*v2[Z];                        \
    if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
    rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
    if(min>rad || max<-rad) return false;

#define AXISTEST_X2(a, b, fa, fb)                  \
    p0 = a*v0[Y] - b*v0[Z];                        \
    p1 = a*v1[Y] - b*v1[Z];                        \
    if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
    rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \
    if(min>rad || max<-rad) return false;

#define AXISTEST_Y02(a, b, fa, fb)                 \
    p0 = -a*v0[X] + b*v0[Z];                       \
    p2 = -a*v2[X] + b*v2[Z];                       \
    if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
    if(min>rad || max<-rad) return false;

#define AXISTEST_Y1(a, b, fa, fb)                  \
    p0 = -a*v0[X] + b*v0[Z];                       \
    p1 = -a*v1[X] + b*v1[Z];                       \
    if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \
    if(min>rad || max<-rad) return false;

#define AXISTEST_Z12(a, b, fa, fb)                 \
    p1 = a*v1[X] - b*v1[Y];                        \
    p2 = a*v2[X] - b*v2[Y];                        \
    if(p2<p1) {min=p2; max=p1;} else {min=p1; max=p2;} \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
    if(min>rad || max<-rad) return false;

#define AXISTEST_Z0(a, b, fa, fb)                  \
    p0 = a*v0[X] - b*v0[Y];                        \
    p1 = a*v1[X] - b*v1[Y];                        \
    if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
    rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \
    if(min>rad || max<-rad) return false;

static int planeBoxOverlap(const float normal[3], const float vert[3], const float maxbox[3])
{
    int q;
    float vmin[3], vmax[3], v;
    for (q = X; q <= Z; q++)
    {
        v = vert[q];
        if (normal[q] > 0.0f)
        {
            vmin[q] = -maxbox[q] - v;
            vmax[q] = maxbox[q] - v;
        }
        else
        {
            vmin[q] = maxbox[q] - v;
            vmax[q] = -maxbox[q] - v;
        }
    }
    if (normal[0] * vmin[0] + normal[1] * vmin[1] + normal[2] * vmin[2] > 0.0f) return 0;
    if (normal[0] * vmax[0] + normal[1] * vmax[1] + normal[2] * vmax[2] >= 0.0f) return 1;
    return 0;
}

bool NavigationSystem::TriBoxOverlap(const float boxcenter[3], const float boxhalfsize[3], const float triverts[3][3])
{
    float v0[3], v1[3], v2[3];
    float min, max, p0, p1, p2, rad, fex, fey, fez;
    float normal[3], e0[3], e1[3], e2[3];

    // Move triangle into box centered coordinate system
    v0[0] = triverts[0][0] - boxcenter[0]; v0[1] = triverts[0][1] - boxcenter[1]; v0[2] = triverts[0][2] - boxcenter[2];
    v1[0] = triverts[1][0] - boxcenter[0]; v1[1] = triverts[1][1] - boxcenter[1]; v1[2] = triverts[1][2] - boxcenter[2];
    v2[0] = triverts[2][0] - boxcenter[0]; v2[1] = triverts[2][1] - boxcenter[1]; v2[2] = triverts[2][2] - boxcenter[2];
    
    // Compute triangle edges
    e0[0] = v1[0] - v0[0]; e0[1] = v1[1] - v0[1]; e0[2] = v1[2] - v0[2];
    e1[0] = v2[0] - v1[0]; e1[1] = v2[1] - v1[1]; e1[2] = v2[2] - v1[2];
    e2[0] = v0[0] - v2[0]; e2[1] = v0[1] - v2[1]; e2[2] = v0[2] - v2[2];

    // Test the 9 axes given by the cross products of the edges of the box and the edges of the triangle
    fex = fabsf(e0[X]); fey = fabsf(e0[Y]); fez = fabsf(e0[Z]);
    AXISTEST_X01(e0[Z], e0[Y], fez, fey)
    AXISTEST_Y02(e0[Z], e0[X], fez, fex)
    AXISTEST_Z12(e0[Y], e0[X], fey, fex)

    fex = fabsf(e1[X]); fey = fabsf(e1[Y]); fez = fabsf(e1[Z]);
    AXISTEST_X01(e1[Z], e1[Y], fez, fey)
    AXISTEST_Y02(e1[Z], e1[X], fez, fex)
    AXISTEST_Z0(e1[Y], e1[X], fey, fex)

    fex = fabsf(e2[X]); fey = fabsf(e2[Y]); fez = fabsf(e2[Z]);
    AXISTEST_X2(e2[Z], e2[Y], fez, fey)
    AXISTEST_Y1(e2[Z], e2[X], fez, fex)
    AXISTEST_Z12(e2[Y], e2[X], fey, fex)

    // Test the 3 axes corresponding to the box axes
    FINDMINMAX(v0[X], v1[X], v2[X], min, max)
    if (min > boxhalfsize[X] || max < -boxhalfsize[X]) return false;

    FINDMINMAX(v0[Y], v1[Y], v2[Y], min, max)
    if (min > boxhalfsize[Y] || max < -boxhalfsize[Y]) return false;
    
    FINDMINMAX(v0[Z], v1[Z], v2[Z], min, max)
    if (min > boxhalfsize[Z] || max < -boxhalfsize[Z]) return false;

    // Test the axis corresponding to the triangle's normal
    normal[0] = e0[Y] * e1[Z] - e0[Z] * e1[Y];
    normal[1] = e0[Z] * e1[X] - e0[X] * e1[Z];
    normal[2] = e0[X] * e1[Y] - e0[Y] * e1[X];
    if (!planeBoxOverlap(normal, v0, boxhalfsize)) return false;

    return true; // Box and triangle overlap
}
