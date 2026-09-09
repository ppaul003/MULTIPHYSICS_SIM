#ifdef _WIN32
#include <direct.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include <vector>
#include <chrono>
#include <fstream>
#include <string>
#include <cctype>
#include <sstream>
#include <iomanip>

#include "kernel.h"
#include "EuclidEngineEM.h"

#include <algorithm>
#include <cstdio>

#ifdef _WIN32

namespace {

    HICON g_vitruGenLargeIcon = nullptr;
    HICON g_vitruGenSmallIcon = nullptr;

    bool applyVitruGenIconFromFile(
        const char* iconFilename) {

        if (!iconFilename || iconFilename[0] == '\0') {

            return false;
        }

        // Immediately after glutCreateWindow(), this should
        // resolve to the FreeGLUT window.
        HWND windowHandle = GetActiveWindow();

        // Fallback lookup using the original window title.
        if (!windowHandle) {

            windowHandle =
                FindWindowA(nullptr, "NDMSM Sytem Ver0.0.0");
        }

        if (!windowHandle) {

            printf(
                "[EuclidEngine] WARNING: "
                "Could not locate the VitruGen window.\n"
            );

            return false;
        }

        const int largeWidth = GetSystemMetrics(SM_CXICON);
        const int largeHeight = GetSystemMetrics(SM_CYICON);
        const int smallWidth = GetSystemMetrics(SM_CXSMICON);
        const int smallHeight = GetSystemMetrics(SM_CYSMICON);

        g_vitruGenLargeIcon =
            static_cast<HICON>(
                LoadImageA(
                    nullptr,
                    iconFilename,
                    IMAGE_ICON,
                    largeWidth,
                    largeHeight,
                    LR_LOADFROMFILE)
                );

        g_vitruGenSmallIcon =
            static_cast<HICON>(
                LoadImageA(
                    nullptr,
                    iconFilename,
                    IMAGE_ICON,
                    smallWidth,
                    smallHeight,
                    LR_LOADFROMFILE
                )
                );

        if (!g_vitruGenLargeIcon &&
            !g_vitruGenSmallIcon) {

            printf(
                "[EuclidEngine] WARNING: "
                "Could not load app icon: %s\n",
                iconFilename
            );

            return false;
        }

        if (g_vitruGenLargeIcon) {

            SendMessageA(
                windowHandle,
                WM_SETICON,
                ICON_BIG,
                reinterpret_cast<LPARAM>(g_vitruGenLargeIcon)
            );
        }

        if (g_vitruGenSmallIcon) {

            SendMessageA(
                windowHandle,
                WM_SETICON,
                ICON_SMALL,
                reinterpret_cast<LPARAM>(g_vitruGenSmallIcon)
            );
        }

        printf(
            "[EuclidEngine] Application icon loaded: %s\n",
            iconFilename
        );

        return true;
    }

    void releaseVitruGenWindowIcons() {

        if (g_vitruGenLargeIcon) {

            DestroyIcon(g_vitruGenLargeIcon);
            g_vitruGenLargeIcon = nullptr;
        }

        if (g_vitruGenSmallIcon) {

            DestroyIcon(g_vitruGenSmallIcon);
            g_vitruGenSmallIcon = nullptr;
        }
    }

} // namespace

#endif

EuclidEngine* EuclidEngine::s_instance = nullptr;

EuclidEngine::~EuclidEngine() {
    shutdown();
}

bool EuclidEngine::init(int argc, char** argv) {
    std::printf("Anaheim Systems Dynamics\n");
    std::printf("NDMMS System starting...\n");
    std::printf("Diagnostic + GRID_2D/GRID_3D transition host only.\n\n");

    s_instance = this;

    initGL(&argc, argv);
    initRenderer();

    if (!initWorkspaceHost()) {
        std::printf("[EuclidEngine] ERROR: workspace host initialization failed.\n");
        return false;
    }

    glutDisplayFunc(&EuclidEngine::sDisplay);
    glutReshapeFunc(&EuclidEngine::sReshape);
    glutKeyboardFunc(&EuclidEngine::sKeyboard);
    glutMouseFunc(&EuclidEngine::sMouse);
    glutMotionFunc(&EuclidEngine::sMotion);
    glutIdleFunc(&EuclidEngine::sIdle);
    glutCloseFunc(&EuclidEngine::sClose);

    return true;
}

void EuclidEngine::initGL(int* argc, char** argv) {
    glutInit(argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(kWidth, kHeight);
    glutCreateWindow("NDMSM Sytem Ver0.0.0");

#ifdef _WIN32

    applyVitruGenIconFromFile("anaheim.ico");

#endif

    glewExperimental = GL_TRUE;
    const GLenum glewResult = glewInit();
    if (glewResult != GLEW_OK) {
        std::printf("[EuclidEngine] WARNING: GLEW initialization failed: %s\n",
            glewGetErrorString(glewResult));
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.15f, 1.0f);

    m_viewport.resize(kWidth, kHeight);
    m_viewport.applyPerspective(60.0f);
}

void EuclidEngine::initRenderer() {
    if (m_renderer) return;

    m_renderer = new EuclidRenderer();
    m_renderer->setWindowSize(m_viewport.getWidth(), m_viewport.getHeight());
    m_renderer->setFOV(60.0f);
    m_renderer->setSimBoxSize(4);

    std::printf("[EuclidEngine] Grid-only EuclidRenderer initialized.\n");
}

bool EuclidEngine::initWorkspaceHost() {
    if (!m_renderer) return false;

    WorkspaceServices services;
    services.renderer = m_renderer;
    services.arbiter = &m_arbiter;
    services.viewport = &m_viewport;
    services.camera = &m_camera;

    m_arbiter.setApplicationLayer(TheArbiter::ApplicationLayer::GLOBAL_SHELL);
    m_arbiter.setWorkspaceDomain(TheArbiter::WorkspaceDomain::NONE);
    m_arbiter.setActiveWorkspace(TheArbiter::WorkspaceId::DIAGNOSTIC);
    m_camera.setBehaviorMode(CameraProcessor::CAM_MENU_PREVIEW);

    return m_tesseract.initialize(services);
}

WorkspaceFrameContext
EuclidEngine::buildWorkspaceFrameContext(float deltaTime) const {
    WorkspaceFrameContext frame;
    frame.deltaTime = deltaTime;
    frame.elapsedTime = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.001f;
    frame.viewportWidth = m_viewport.getWidth();
    frame.viewportHeight = m_viewport.getHeight();
    frame.displayEnabled = m_displayEnabled;
    return frame;
}

void EuclidEngine::run() {
    if (!m_exiting) glutMainLoop();
}

void EuclidEngine::shutdown() {
    if (m_cleaned) return;
    m_cleaned = true;

    m_tesseract.shutdown();

    delete m_renderer;
    m_renderer = nullptr;
    s_instance = nullptr;
}

void EuclidEngine::requestExit() {
    if (m_exiting) return;
    m_exiting = true;
    glutLeaveMainLoop();
}

void EuclidEngine::computeFPS() {
    static int frameCount = 0;
    static int previousTime = glutGet(GLUT_ELAPSED_TIME);

    ++frameCount;
    const int currentTime = glutGet(GLUT_ELAPSED_TIME);
    const int elapsed = currentTime - previousTime;
    if (elapsed < 1000) return;

    const float fps = static_cast<float>(frameCount) * 1000.0f /
        static_cast<float>(elapsed);

    char title[128];
    std::snprintf(title, sizeof(title), "NDMMS Sytem Ver0.0.0 : %.1f fps", fps);
    glutSetWindowTitle(title);

    frameCount = 0;
    previousTime = currentTime;
}

void EuclidEngine::sDisplay() {
    if (s_instance) s_instance->onDisplay();
}

void EuclidEngine::sReshape(int w, int h) {
    if (s_instance) s_instance->onReshape(w, h);
}

void EuclidEngine::sKeyboard(unsigned char key, int x, int y) {
    if (s_instance) s_instance->onKeyboard(key, x, y);
}

void EuclidEngine::sMouse(int button, int state, int x, int y) {

    if (s_instance)
        s_instance->onMouse(button, state, x, y);
}

void EuclidEngine::sMotion(int x, int y) {

    if (s_instance)
        s_instance->onMotion(x, y);
}

void EuclidEngine::sIdle() {
    if (s_instance) s_instance->onIdle();
}

void EuclidEngine::sClose() {
    if (s_instance) s_instance->onClose();
}

void EuclidEngine::onReshape(int w, int h) {
    m_viewport.resize(w, h);
    m_viewport.applyPerspective(60.0f);

    if (m_renderer)
        m_renderer->setWindowSize(w, h);

    glutPostRedisplay();
}

void EuclidEngine::onDisplay() {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_viewport.applyPerspective(60.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    m_camera.updateLag();
    m_camera.applyCameraTransform();

    const WorkspaceFrameContext frame = buildWorkspaceFrameContext(0.0f);
    m_tesseract.render(frame);
    m_viewport.drawOverlay(m_tesseract.presentation());

    glutSwapBuffers();
}

void EuclidEngine::onKeyboard(unsigned char key, int x, int y) {
    const KeyboardInput::KeyEvent event = m_keyboard.onKey(key, x, y);
    const TheArbiter::ArbiterResult result = m_arbiter.routeKeyboard(event);

    if (result.arbiterCommand == TheArbiter::ArbiterCommand::CMD_EXIT) {
        requestExit();
        return;
    }

    if (result.hasWorkspaceInput) {
        m_tesseract.handleInput(result.workspaceInput);
        glutPostRedisplay();
    }
}

void EuclidEngine::onMouse(int button, int state, int x, int y) {

    m_mouse.onButton(button, state, x, y);

    // Never manipulate camera during automatic transition.
    if (m_tesseract.domainTransitionActive())
        return;

    // This sprint: camera interaction belongs to Layer 1.
    if (!m_arbiter.isWorkspaceLayer())
        return;

    // FreeGLUT wheel up/down.
    if (state == GLUT_DOWN) {

        if (button == 3 && m_camera.zoomEnabled()) {
            m_camera.zoom(+0.05f);
        }

        else if (button == 4 && m_camera.zoomEnabled()) {
            m_camera.zoom(-0.05f);
        }
    }

    glutPostRedisplay();
}

void EuclidEngine::onMotion(int x, int y) {

    int dx = 0;
    int dy = 0;

    if (!m_mouse.onMotion(x, y, dx, dy))
        return;

    if (m_tesseract.domainTransitionActive())
        return;

    if (!m_arbiter.isWorkspaceLayer())
        return;

    if (!m_camera.orbitEnabled())
        return;

    m_camera.orbit(
        static_cast<float>(dx),
        static_cast<float>(dy)
    );

    glutPostRedisplay();
}

void EuclidEngine::onIdle() {
    static int previousTimeMs = glutGet(GLUT_ELAPSED_TIME);

    const int currentTimeMs = glutGet(GLUT_ELAPSED_TIME);
    const int elapsedMs = currentTimeMs - previousTimeMs;
    previousTimeMs = currentTimeMs;

    float deltaTime = static_cast<float>(elapsedMs) * 0.001f;
    deltaTime = (std::min)(deltaTime, 0.050f);

    const WorkspaceFrameContext frame = buildWorkspaceFrameContext(deltaTime);
    m_tesseract.update(frame);

    computeFPS();
    glutPostRedisplay();
}

void EuclidEngine::onClose() {
    shutdown();
}
