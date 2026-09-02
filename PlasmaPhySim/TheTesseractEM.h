#ifndef NDMSM_TESSERACT_EM_H
#define NDMSM_TESSERACT_EM_H

#include <GL/glew.h>
#include <cstddef>
#include <vector>
#include <vector_types.h>

#include "IWorkspaceEM.h"
#include "WorkspaceContextEM.h"
#include "WorkspaceInputEM.h"
#include "WorkspacePresentationEM.h"
#include "TheArbiterEM.h"

#include "DiagnosticIdleEM.h"
#include "ParticleSimWorkspace.h"
#include "Graph3DWorkspace.h"
#include "ANNDesignWorkspace.h"
#include "Graph2DWorkspace.h"
#include "Heat2DWorkspace.h"

class Tesseract {
public:
    enum class DomainTransitionPhase {
        NONE = 0,

        ENTER_DOMAIN_VISUAL,
        ENTER_DOMAIN_READY,
        ENTER_CAMERA,

        ENTER_2D_CAMERA_Z,
        ENTER_2D_CAMERA_X,

        EXIT_CAMERA,
        EXIT_2D_CAMERA_X,
        EXIT_2D_CAMERA_Z,
        EXIT_DOMAIN_VISUAL
    };

    bool initialize(WorkspaceServices services);
    void shutdown();

    void update(const WorkspaceFrameContext& frame);
    void render(const WorkspaceFrameContext& frame);
    bool handleInput(const WorkspaceInputEvent& event);

    WorkspacePresentation presentation() const;

    void processNavigationRequest();
    void updateDomainTransition(const WorkspaceFrameContext& frame);

    bool domainTransitionActive() const {
        return m_domainTransitionPhase != DomainTransitionPhase::NONE;
    }

private:
    void synchronizeActiveCartridge();
    void activateCartridge(IWorkspace* workspace, const char* name);

    static constexpr float kGrid3DCamTransDuration = 0.75f;
    static constexpr float kGridReadyHoldDuration = 0.35f;
    static constexpr float kGrid2DCamCenterDuration = 0.45f;
    static constexpr float kGrid2DCamLateralDuration = 0.30f;

    WorkspaceServices m_services;
    DiagnosticIdle m_diagnosticIdle;

    ParticleSimWorkspace m_particleSimWorkspace;

    Graph3DWorkspace m_graph3DWorkspace;
    ANNDesignWorkspace m_annDesignWorkspace;

    Graph2DWorkspace m_graph2DWorkspace;
    Heat2DWorkspace m_heat2DWorkspace;

    IWorkspace* m_activeWorkspace = nullptr;

    DomainTransitionPhase m_domainTransitionPhase = DomainTransitionPhase::NONE;
    TheArbiter::WorkspaceDomain m_transitionDomain = TheArbiter::WorkspaceDomain::NONE;
    float m_transitionPhaseElapsed = 0.0f;
};

#endif
