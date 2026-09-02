#ifndef NDMSM_WORKSPACE_CONTEXT_EM_H
#define NDMSM_WORKSPACE_CONTEXT_EM_H

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
