#ifndef NDMSM_CAMERA_EM_H
#define NDMSM_CAMERA_EM_H

#include <GL/glew.h>

class CameraProcessor {
public:
    enum CameraBehaviorMode {
        CAM_MENU_PREVIEW = 0,
        CAM_STANDARD_3D,
        CAM_STANDARD_2D
    };

    CameraProcessor();

    void updateLag();
    void applyCameraTransform();

    void setBehaviorMode(CameraBehaviorMode mode);
    CameraBehaviorMode getBehaviorMode() const { return m_behaviorMode; }

    void beginTransitionToStandard3D(float duration = 0.75f);
    void beginTransitionToCentered2D(float duration = 0.45f);
    void beginTransitionToStandard2D(float duration = 0.30f);
    void beginTransitionToPreMenu2D(float duration = 0.45f);
    void beginTransitionToMenu(float duration = 0.75f);

    void updatePoseTransition(float deltaTime);
    bool poseTransitionActive() const { return m_poseTransitionActive; }

private:
    void beginTransitionToPose(
        float tx, float ty, float tz,
        float rx, float ry, float rz,
        float duration);

    void setMenuPoseTarget();
    void setStandard3DTarget();
    void setStandard2DTarget();

private:
    static constexpr float kInertia = 0.10f;

    CameraBehaviorMode m_behaviorMode = CAM_MENU_PREVIEW;

    bool m_poseTransitionActive = false;
    float m_poseTransitionElapsed = 0.0f;
    float m_poseTransitionDuration = 0.75f;

    float m_poseStartTrans[3]{};
    float m_poseStartRot[3]{};
    float m_poseTargetTrans[3]{};
    float m_poseTargetRot[3]{};

    float m_cameraTrans[3];
    float m_cameraRot[3];
    float m_cameraTransLag[3];
    float m_cameraRotLag[3];
};

#endif
