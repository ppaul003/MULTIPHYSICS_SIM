#include "CameraEM.h"

#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/freeglut.h>

using namespace std;

namespace {
    constexpr float kMenuX = 1.65f;
    constexpr float kMenuY = 0.00f;
    constexpr float kMenuZ = -8.00f;
    constexpr float kMenuPitch = 18.0f;

    constexpr float kStandard3DZ = -5.00f;

    constexpr float kStandard2DX = 1.40f;
    constexpr float kStandard2DY = 0.00f;
    constexpr float kStandard2DZ = -6.00f;
    constexpr float kPreMenu2DZ = -8.00f;

    float smoothStep01(float t) {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;
        return t * t * (3.0f - 2.0f * t);
    }

    float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }
}

CameraProcessor::CameraProcessor() :
    m_cameraTrans{ kMenuX, kMenuY, kMenuZ },
    m_cameraRot{ kMenuPitch, 0.0f, 0.0f },
    m_cameraTransLag{ kMenuX, kMenuY, kMenuZ },
    m_cameraRotLag{ kMenuPitch, 0.0f, 0.0f } {
}

void CameraProcessor::updateLag() {
    if (m_poseTransitionActive) return;

    for (int c = 0; c < 3; c++) {
        m_cameraTransLag[c] +=
            (m_cameraTrans[c] - m_cameraTransLag[c]) * kInertia;
        m_cameraRotLag[c] +=
            (m_cameraRot[c] - m_cameraRotLag[c]) * kInertia;
    }
}

void CameraProcessor::applyCameraTransform() {
    glTranslatef(
        m_cameraTransLag[0],
        m_cameraTransLag[1],
        m_cameraTransLag[2]);

    glRotatef(m_cameraRotLag[0], 1.0f, 0.0f, 0.0f);
    glRotatef(m_cameraRotLag[1], 0.0f, 1.0f, 0.0f);
    glRotatef(m_cameraRotLag[2], 0.0f, 0.0f, 1.0f);
}

void CameraProcessor::setMenuPoseTarget() {
    m_cameraTrans[0] = kMenuX;
    m_cameraTrans[1] = kMenuY;
    m_cameraTrans[2] = kMenuZ;
    m_cameraRot[0] = kMenuPitch;
    m_cameraRot[1] = 0.0f;
    m_cameraRot[2] = 0.0f;
}

void CameraProcessor::setStandard3DTarget() {
    m_cameraTrans[0] = 0.0f;
    m_cameraTrans[1] = 0.0f;
    m_cameraTrans[2] = kStandard3DZ;
    m_cameraRot[0] = 0.0f;
    m_cameraRot[1] = 0.0f;
    m_cameraRot[2] = 0.0f;
}

void CameraProcessor::setStandard2DTarget() {
    m_cameraTrans[0] = kStandard2DX;
    m_cameraTrans[1] = kStandard2DY;
    m_cameraTrans[2] = kStandard2DZ;
    m_cameraRot[0] = 0.0f;
    m_cameraRot[1] = 0.0f;
    m_cameraRot[2] = 0.0f;
}

void CameraProcessor::setBehaviorMode(CameraBehaviorMode mode) {
    m_behaviorMode = mode;

    switch (mode) {
    case CAM_STANDARD_3D:
        setStandard3DTarget();
        break;
    case CAM_STANDARD_2D:
        setStandard2DTarget();
        break;
    case CAM_MENU_PREVIEW:
    default:
        setMenuPoseTarget();
        break;
    }
}

void CameraProcessor::beginTransitionToPose(
    float tx, float ty, float tz,
    float rx, float ry, float rz,
    float duration) {

    for (int c = 0; c < 3; c++) {
        m_poseStartTrans[c] = m_cameraTransLag[c];
        m_poseStartRot[c] = m_cameraRotLag[c];
        m_cameraTrans[c] = m_poseStartTrans[c];
        m_cameraRot[c] = m_poseStartRot[c];
    }

    m_poseTargetTrans[0] = tx;
    m_poseTargetTrans[1] = ty;
    m_poseTargetTrans[2] = tz;
    m_poseTargetRot[0] = rx;
    m_poseTargetRot[1] = ry;
    m_poseTargetRot[2] = rz;

    m_poseTransitionElapsed = 0.0f;
    m_poseTransitionDuration = duration;

    if (duration <= 0.0f) {
        for (int c = 0; c < 3; c++) {
            m_cameraTrans[c] = m_poseTargetTrans[c];
            m_cameraRot[c] = m_poseTargetRot[c];
            m_cameraTransLag[c] = m_poseTargetTrans[c];
            m_cameraRotLag[c] = m_poseTargetRot[c];
        }
        m_poseTransitionActive = false;
        return;
    }

    m_poseTransitionActive = true;
}

void CameraProcessor::beginTransitionToStandard3D(float duration) {
    beginTransitionToPose(
        0.0f, 0.0f, kStandard3DZ,
        0.0f, 0.0f, 0.0f,
        duration
    );
}

void CameraProcessor::beginTransitionToCentered2D(float duration) {
    beginTransitionToPose(
        0.0f, 0.0f, kStandard2DZ,
        0.0f, 0.0f, 0.0f,
        duration);
}

void CameraProcessor::beginTransitionToStandard2D(float duration) {
    beginTransitionToPose(
        kStandard2DX, kStandard2DY, kStandard2DZ,
        0.0f, 0.0f, 0.0f,
        duration
    );
}

void CameraProcessor::beginTransitionToPreMenu2D(float duration) {
    beginTransitionToPose(
        0.0f, 0.0f, kPreMenu2DZ,
        0.0f, 0.0f, 0.0f,
        duration
    );
}

void CameraProcessor::beginTransitionToMenu(float duration) {
    beginTransitionToPose(
        kMenuX, kMenuY, kMenuZ,
        kMenuPitch, 0.0f, 0.0f,
        duration
    );
}

void CameraProcessor::updatePoseTransition(float deltaTime) {
    if (!m_poseTransitionActive) return;
    if (deltaTime < 0.0f) deltaTime = 0.0f;

    m_poseTransitionElapsed += deltaTime;

    if (m_poseTransitionDuration <= 0.0f) {
        m_poseTransitionActive = false;
        return;
    }

    const float rawT = m_poseTransitionElapsed / m_poseTransitionDuration;
    const float t = smoothStep01(rawT);

    for (int c = 0; c < 3; c++) {
        const float translation = lerp(
            m_poseStartTrans[c], m_poseTargetTrans[c], t);
        const float rotation = lerp(
            m_poseStartRot[c], m_poseTargetRot[c], t);

        m_cameraTrans[c] = translation;
        m_cameraTransLag[c] = translation;
        m_cameraRot[c] = rotation;
        m_cameraRotLag[c] = rotation;
    }

    if (rawT >= 1.0f) {
        for (int c = 0; c < 3; c++) {
            m_cameraTrans[c] = m_poseTargetTrans[c];
            m_cameraTransLag[c] = m_poseTargetTrans[c];
            m_cameraRot[c] = m_poseTargetRot[c];
            m_cameraRotLag[c] = m_poseTargetRot[c];
        }
        m_poseTransitionElapsed = m_poseTransitionDuration;
        m_poseTransitionActive = false;
    }
}

void CameraProcessor::orbit(float dx, float dy) {
    if (!orbitEnabled()) return;

    // Mouse Y -> pitch
    m_cameraRot[0] += dy / 5.0f;

    // Mouse X -> yaw
    m_cameraRot[1] += dx / 5.0f;
}

void CameraProcessor::zoom(float amount) {
    if (!zoomEnabled()) return;

    const float distance = max(1.0f, fabs(m_cameraTrans[2]));

    m_cameraTrans[2] += amount * distance;
    m_cameraTrans[2] = clamp(m_cameraTrans[2], -30.0f, -1.5f);
}
