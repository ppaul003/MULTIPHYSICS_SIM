#ifndef NDMSM_DIAGNOSTIC_IDLE_EM_H
#define NDMSM_DIAGNOSTIC_IDLE_EM_H

#include "IWorkspaceEM.h"

class TheArbiter;

class DiagnosticIdle : public IWorkspace {
public:
    enum class VisualTransitionState {
        Idle = 0,

        Grid3D_OrientToFront,
        Grid3D_CaptureSlice,
        Grid3D_HoldCenter,
        Grid3D_ReleaseSlice,

        Grid2D_OrientToFront,
        Grid2D_WaitForXYStart,
        Grid2D_SweepToFront,
        Grid2D_HoldFront,
        Grid2D_ReturnSweep,

        Mulphy_OrientToFront,
        Mulphy_WaitForXYStart,
        Mulphy_SweepToMajor,
        Mulphy_HoldMajor,
        Mulphy_AxisToOrigin,
        Mulphy_HoldVolume,
        Mulphy_ReturnAxisToCenter,
        Mulphy_ReturnSweepToMajor,
        Mulphy_ReturnHoldMajor
    };

    bool initialize(WorkspaceServices& services) override;
    void enter(WorkspaceServices& services) override;
    void exit(WorkspaceServices& services) override;

    void update(
        const WorkspaceFrameContext& frame,
        WorkspaceServices& services) override;

    void render(
        const WorkspaceFrameContext& frame,
        WorkspaceServices& services) override;

    bool handleInput(
        const WorkspaceInputEvent& input,
        WorkspaceServices& services) override;

    WorkspacePresentation buildPresentation() const override;

    void beginGrid3DEnterTransition();
    void beginGrid3DReturnTransition();
    void beginGrid2DEnterTransition();
    void beginGrid2DReturnTransition();
    void beginMulphyEnterTransition();
    void beginMulphyReturnTransition();

    bool grid3DEnterVisualComplete() const { return m_grid3DEnterComplete; }
    bool grid3DReturnVisualComplete() const { return m_grid3DReturnComplete; }
    bool grid2DEnterVisualComplete() const { return m_grid2DEnterComplete; }
    bool grid2DReturnVisualComplete() const { return m_grid2DReturnComplete; }
    bool mulphyEnterVisualComplete() const { return m_mulphyEnterComplete; }
    bool mulphyReturnVisualComplete() const { return m_mulphyReturnComplete; }

private:
    enum class GlobalShellRow {
        Environment = 0,
        SimulationBox,
        SimulationUnit,
        Configure,
        Count
    };

    const char* selectedEnvironmentName() const;
    const char* selectedUnitMeasurementName() const;
    void cycleEnvironment(int direction);
    void cycleUnitMeasurement(int direction);
    void moveGlobalShellCursor(int direction);
    void adjustGlobalShellValue(int direction);

private:
    static constexpr float kPreviewRotationSpeed = 25.0f;
    static constexpr float kSliceCycleSpeed = 0.35f;
    static constexpr float kTransitionRotationSpeed = 120.0f;
    static constexpr float kTransitionSliceSpeed = 1.40f;
    static constexpr float kMulphyMajorHoldDuration = 0.12f;
    static constexpr float kMulphyAxisTransitionDuration = 0.45f;

    GlobalShellRow m_activeShellRow = GlobalShellRow::Environment;
    TheArbiter* m_arbiter = nullptr;

    int m_requestedSimBoxSize = 4;

    VisualTransitionState m_visualTransition = VisualTransitionState::Idle;

    bool m_grid3DEnterComplete = false;
    bool m_grid3DReturnComplete = false;
    bool m_grid2DEnterComplete = false;
    bool m_grid2DReturnComplete = false;
    bool m_mulphyEnterComplete = false;
    bool m_mulphyReturnComplete = false;

    float m_previewRotationDegrees = 0.0f;
    float m_sliceTravel = 0.0f;
    float m_targetRotationDegrees = 0.0f;
    float m_targetSliceTravel = 0.0f;
    float m_grid2DPlaneProgress = 0.0f;
    float m_mulphyPlaneProgress = 0.0f;
    float m_mulphyClearedProgress = 0.0f;
    float m_mulphyAxisProgress = 0.0f;
    float m_mulphyMajorHoldElapsed = 0.0f;
    int m_transitionGridDimension = 64;
    int m_transitionGridMajorEvery = 8;
    int m_mulphyMajorCount = 8;
    int m_mulphyTargetMajorIndex = 1;
};

#endif
