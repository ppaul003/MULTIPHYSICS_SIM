#ifndef NDMSM_EUCLID_ENGINE_EM_H
#define NDMSM_EUCLID_ENGINE_EM_H

#ifdef _WIN32
#include <Windows.h>
#endif

#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/freeglut.h>

#include <helper_functions.h>
#include <helper_cuda.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <vector>
#include <future>
#include <filesystem>
#include <string>

#include "InteractionsEM.h"
#include "CameraEM.h"
#include "ViewPortEM.h"
#include "TheArbiterEM.h"
#include "TheTesseractEM.h"
#include "rendererEM_Euclid.h"

class EuclidEngine {
public:
    EuclidEngine() = default;
    ~EuclidEngine();

    bool init(int argc, char** argv);
    void run();
    void shutdown();

private:
    static EuclidEngine* s_instance;

    static void sDisplay();
    static void sReshape(int w, int h);
    static void sKeyboard(unsigned char key, int x, int y);
    static void sMouse(int button, int state, int x, int y);
    static void sMotion(int x, int y);
    static void sIdle();
    static void sClose();

    void initGL(int* argc, char** argv);
    void initRenderer();
    bool initWorkspaceHost();

    WorkspaceFrameContext buildWorkspaceFrameContext(float deltaTime) const;

    void onDisplay();
    void onReshape(int w, int h);
    void onKeyboard(unsigned char key, int x, int y);
    void onMouse(int button, int state, int x, int y);
    void onMotion(int x, int y);
    void onIdle();
    void onClose();

    void computeFPS();
    void requestExit();

private:
    static constexpr unsigned int kWidth = 1920;
    static constexpr unsigned int kHeight = 1080;

    TheArbiter m_arbiter;
    Tesseract m_tesseract;
    ViewPort m_viewport;
    CameraProcessor m_camera;
    KeyboardInput m_keyboard;
    MouseInput m_mouse;

    EuclidRenderer* m_renderer = nullptr;

    bool m_displayEnabled = true;
    bool m_exiting = false;
    bool m_cleaned = false;
};

#endif
