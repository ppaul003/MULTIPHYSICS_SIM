#ifndef NDMSM_WORKSPACE_CONTEXT_EM_H
#define NDMSM_WORKSPACE_CONTEXT_EM_H

#include <glm/glm.hpp>

struct SpatialVoxelRegion {
    unsigned int id = 0;
    glm::ivec3 index = glm::ivec3(0);
    glm::vec3 minimum = glm::vec3(0.0f);
    glm::vec3 maximum = glm::vec3(0.0f);
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 halfExtent = glm::vec3(0.0f);
    float volumeM3 = 0.0f;
};

// Physical workspace partition shared by particle and future multiphysics
// cartridges. IDs are X-fastest, followed by Y and then Z.
struct SpatialVoxelGrid3D {
    glm::ivec3 dimensions = glm::ivec3(8, 8, 8);
    glm::vec3 origin = glm::vec3(-2.0f, -2.0f, -2.0f);
    float voxelEdgeM = 0.5f;

    unsigned int voxelCount() const {
        if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
            return 0;
        }

        return static_cast<unsigned int>(
            dimensions.x * dimensions.y * dimensions.z
        );
    }

    bool region(unsigned int id, SpatialVoxelRegion& result) const {
        const unsigned int count = voxelCount();
        if (id >= count || voxelEdgeM <= 0.0f) {
            return false;
        }

        const unsigned int dimensionX =
            static_cast<unsigned int>(dimensions.x);
        const unsigned int dimensionY =
            static_cast<unsigned int>(dimensions.y);

        const unsigned int x = id % dimensionX;
        const unsigned int y = (id / dimensionX) % dimensionY;
        const unsigned int z = id / (dimensionX * dimensionY);

        result.id = id;
        result.index = glm::ivec3(
            static_cast<int>(x),
            static_cast<int>(y),
            static_cast<int>(z)
        );
        result.minimum = origin + glm::vec3(
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(z)
        ) * voxelEdgeM;
        result.maximum = result.minimum + glm::vec3(voxelEdgeM);
        result.center = (result.minimum + result.maximum) * 0.5f;
        result.halfExtent = glm::vec3(voxelEdgeM * 0.5f);
        result.volumeM3 = voxelEdgeM * voxelEdgeM * voxelEdgeM;
        return true;
    }
};

class EuclidRenderer;
class TheArbiter;
class ViewPort;
class CameraProcessor;

struct WorkspaceFrameContext {
    float deltaTime = 0.0f;
    float elapsedTime = 0.0f;
    int viewportWidth = 1920;
    int viewportHeight = 1080;
    bool displayEnabled = true;
};

struct WorkspaceServices {
    EuclidRenderer* renderer = nullptr;
    TheArbiter* arbiter = nullptr;
    ViewPort* viewport = nullptr;
    CameraProcessor* camera = nullptr;
};

#endif
