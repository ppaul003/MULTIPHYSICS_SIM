#include "TheTesseractEM.h"
#include "CameraEM.h"

#include <cstdio>

bool MultiphysicsSimPlaceholderWorkspace::initialize(WorkspaceServices& services) {
    m_arbiter = services.arbiter;
    return m_arbiter != nullptr;
}

void MultiphysicsSimPlaceholderWorkspace::enter(WorkspaceServices& services) {
    if (!m_arbiter) m_arbiter = services.arbiter;
    m_active = true;
}

void MultiphysicsSimPlaceholderWorkspace::exit(WorkspaceServices& services) {
    (void)services;
    m_active = false;
}

void MultiphysicsSimPlaceholderWorkspace::update(
    const WorkspaceFrameContext& frame,
    WorkspaceServices& services) {
    (void)frame;
    (void)services;
}

void MultiphysicsSimPlaceholderWorkspace::render(
    const WorkspaceFrameContext& frame,
    WorkspaceServices& services) {
    (void)frame;
    (void)services;
}

bool MultiphysicsSimPlaceholderWorkspace::handleInput(
    const WorkspaceInputEvent& input,
    WorkspaceServices& services) {

    if (!m_active) return false;
    if (!m_arbiter) m_arbiter = services.arbiter;
    if (!m_arbiter) return false;

    switch (input.action) {
    case WorkspaceInputAction::Decrease:
    case WorkspaceInputAction::Increase:
    case WorkspaceInputAction::Activate:
        m_arbiter->setActiveWorkspace(
            TheArbiter::WorkspaceId::PARTICLE_SIMULATION);
        return true;
    case WorkspaceInputAction::Back:
        m_arbiter->requestReturnToGlobalShell(
            TheArbiter::WorkspaceDomain::MULPHY_SIM);
        return true;
    case WorkspaceInputAction::Previous:
    case WorkspaceInputAction::Next:
        return true;
    default:
        return false;
    }
}

WorkspacePresentation
MultiphysicsSimPlaceholderWorkspace::buildPresentation() const {
    WorkspacePresentation p;
    p.panelVisible = true;
    p.workspaceName = "LAYER 1 -> MULPHY_SIM WORKSPACE CONFIGURATION";
    p.layerLabel = "MODE: MULTIPHYSICS_SIM";

    WorkspacePanelSection section;
    WorkspacePanelRow selectionRow;
    selectionRow.label = "[1]: MULPHY_SIM SELECTION";
    selectionRow.value = "MULTIPHYSICS_SIM";
    selectionRow.selectable = true;
    selectionRow.selected = true;
    section.rows.push_back(selectionRow);

    WorkspacePanelRow reservedRow;
    reservedRow.label = "MULTIPHYSICS_SIM is reserved for the next pass";
    section.rows.push_back(reservedRow);
    p.sections.push_back(section);

    p.statusLine = "MULTIPHYSICS_SIM is reserved for the next pass";
    p.statusTone = WorkspaceStatusTone::Warning;
    p.footerLine1 = "A/D or E: Select PARTICLE_SIM";
    p.footerLine2 = "Q: Return to Global Shell    ESC: Exit";
    return p;
}

bool Tesseract::initialize(WorkspaceServices services) {
    m_services = services;

    if (!m_diagnosticIdle.initialize(m_services)) {
        std::printf("[Tesseract] ERROR: DiagnosticIdle initialization failed.\n");
        return false;
    }

    if (!m_graph2DWorkspace.initialize(m_services)) {
        std::printf("[Tesseract] ERROR: Graph2DWorkspace initialization failed.\n");
        return false;
    }

    if (!m_heat2DWorkspace.initialize(m_services)) {
        std::printf("[Tesseract] ERROR: Heat2DWorkspace initialization failed.\n");
        return false;
    }

    if (!m_graph3DWorkspace.initialize(m_services)) {
        std::printf("[Tesseract] ERROR: Graph3DWorkspace initialization failed.\n");
        return false;
    }

    if (!m_annDesignWorkspace.initialize(m_services)) {
        std::printf("[Tesseract] ERROR: ANNDesignWorkspace initialization failed.\n");
        return false;
    }

    if (!m_particleSimWorkspace.initialize(m_services)) {
        std::printf("[Tesseract] ERROR: ParticleSimWorkspace initialization failed.\n");
        return false;
    }

    if (!m_multiphysicsPlaceholderWorkspace.initialize(m_services)) {
        std::printf("[Tesseract] ERROR: MULTIPHYSICS_SIM placeholder initialization failed.\n");
        return false;
    }

    m_activeWorkspace = &m_diagnosticIdle;
    m_activeWorkspace->enter(m_services);
    std::printf("[Tesseract] Workspace socket initialized.\n");
    std::printf("[Tesseract] Active workspace: DIAGNOSTIC_IDLE\n");
    return true;
}

void Tesseract::shutdown() {
    if (m_activeWorkspace) m_activeWorkspace->exit(m_services);
    m_activeWorkspace = nullptr;
}

void Tesseract::update(const WorkspaceFrameContext& frame) {
    processNavigationRequest();

    if (domainTransitionActive()) {
        updateDomainTransition(frame);
        return;
    }

    synchronizeActiveCartridge();
    if (m_activeWorkspace)
        m_activeWorkspace->update(frame, m_services);
}

void Tesseract::render(const WorkspaceFrameContext& frame) {
    if (domainTransitionActive()) {
        switch (m_domainTransitionPhase) {
        case DomainTransitionPhase::ENTER_DOMAIN_VISUAL:
        case DomainTransitionPhase::ENTER_DOMAIN_READY:
        case DomainTransitionPhase::ENTER_CAMERA:
        case DomainTransitionPhase::ENTER_2D_CAMERA_Z:
        case DomainTransitionPhase::ENTER_2D_CAMERA_X:
        case DomainTransitionPhase::EXIT_DOMAIN_VISUAL:
            m_diagnosticIdle.render(frame, m_services);
            return;

        case DomainTransitionPhase::EXIT_CAMERA:
        case DomainTransitionPhase::EXIT_2D_CAMERA_X:
        case DomainTransitionPhase::EXIT_2D_CAMERA_Z:
            if (m_activeWorkspace)
                m_activeWorkspace->render(frame, m_services);
            return;

        case DomainTransitionPhase::NONE:
        default:
            break;
        }
    }

    synchronizeActiveCartridge();
    if (m_activeWorkspace)
        m_activeWorkspace->render(frame, m_services);
}

bool Tesseract::handleInput(const WorkspaceInputEvent& event) {
    if (domainTransitionActive()) return true;

    if (!m_activeWorkspace) return false;

    const bool handled = m_activeWorkspace->handleInput(event, m_services);
    processNavigationRequest();

    if (!domainTransitionActive())
        synchronizeActiveCartridge();

    return handled;
}

WorkspacePresentation Tesseract::presentation() const {
    using Domain = TheArbiter::WorkspaceDomain;
    using Phase = DomainTransitionPhase;

    if (!m_activeWorkspace) {
        WorkspacePresentation p;
        p.panelVisible = true;
        p.workspaceName = "PLASMAPHYSIM HOST";
        p.layerLabel = "NO ACTIVE WORKSPACE";
        p.statusLine = "WORKSPACE SOCKET OFFLINE";
        return p;
    }

    if (m_transitionDomain == Domain::GRID_2D) {
        switch (m_domainTransitionPhase) {
        case Phase::ENTER_DOMAIN_VISUAL: {
            WorkspacePresentation p = m_diagnosticIdle.buildPresentation();
            p.statusLine = "AUTO: Transitioning To GRID_2D...";
            p.statusTone = WorkspaceStatusTone::Transition;
            p.statusBlink = true;
            p.frameTone = WorkspaceStatusTone::Transition;
            p.frameBlink = true;
            return p;
        }
        case Phase::ENTER_DOMAIN_READY: {
            WorkspacePresentation p = m_diagnosticIdle.buildPresentation();
            p.statusLine = "READY: GRID_2D Setup Complete.";
            p.statusTone = WorkspaceStatusTone::Ready;
            p.statusBlink = false;
            p.frameTone = WorkspaceStatusTone::Ready;
            p.frameBlink = false;
            return p;
        }
        default:
            break;
        }
    }

    if (m_transitionDomain == Domain::GRID_3D) {
        switch (m_domainTransitionPhase) {
        case Phase::ENTER_DOMAIN_VISUAL: {
            WorkspacePresentation p = m_diagnosticIdle.buildPresentation();
            p.statusLine = "AUTO: Transitioning To GRID_3D...";
            p.statusTone = WorkspaceStatusTone::Transition;
            p.statusBlink = true;
            p.frameTone = WorkspaceStatusTone::Transition;
            p.frameBlink = true;
            return p;
        }
        case Phase::ENTER_DOMAIN_READY: {
            WorkspacePresentation p = m_diagnosticIdle.buildPresentation();
            p.statusLine = "READY: GRID_3D Setup Complete.";
            p.statusTone = WorkspaceStatusTone::Ready;
            p.statusBlink = false;
            p.frameTone = WorkspaceStatusTone::Ready;
            p.frameBlink = false;
            return p;
        }
        case Phase::ENTER_CAMERA: {

            WorkspacePresentation p =
                m_graph3DWorkspace.buildLayer1TransitionPresentation();

            p.frameTone = WorkspaceStatusTone::Ready;
            p.frameBlink = false;
            return p;
        }
        default:
            break;
        }
    }

    if (m_transitionDomain == Domain::MULPHY_SIM) {
        switch (m_domainTransitionPhase) {
        case Phase::ENTER_DOMAIN_VISUAL: {
            WorkspacePresentation p = m_diagnosticIdle.buildPresentation();
            p.statusLine = "AUTO: Transitioning To MULPHY_SIM...";
            p.statusTone = WorkspaceStatusTone::Transition;
            p.statusBlink = true;
            p.frameTone = WorkspaceStatusTone::Transition;
            p.frameBlink = true;
            return p;
        }
        case Phase::ENTER_DOMAIN_READY: {
            WorkspacePresentation p = m_diagnosticIdle.buildPresentation();
            p.statusLine = "READY: MULPHY_SIM Setup Complete.";
            p.statusTone = WorkspaceStatusTone::Ready;
            p.statusBlink = false;
            p.frameTone = WorkspaceStatusTone::Ready;
            p.frameBlink = false;
            return p;
        }
        case Phase::ENTER_CAMERA: {

            WorkspacePresentation p =
                m_particleSimWorkspace.buildLayer1TransitionPresentation();

            p.frameTone = WorkspaceStatusTone::Ready;
            p.frameBlink = false;
            return p;
        }
        default:
            break;
        }
    }

    return m_activeWorkspace->buildPresentation();
}

void Tesseract::processNavigationRequest() {
    if (!m_services.arbiter) return;
    if (domainTransitionActive()) return;
    if (!m_services.arbiter->hasNavigationRequest()) return;

    const TheArbiter::NavigationRequest request =
        m_services.arbiter->takeNavigationRequest();

    using Request = TheArbiter::NavigationRequestType;
    using Domain = TheArbiter::WorkspaceDomain;

    switch (request.type) {
    case Request::ENTER_DOMAIN:
        if (request.domain != Domain::GRID_2D &&
            request.domain != Domain::GRID_3D &&
            request.domain != Domain::MULPHY_SIM)
            return;

        m_transitionDomain = request.domain;
        m_domainTransitionPhase = DomainTransitionPhase::ENTER_DOMAIN_VISUAL;

        if (request.domain == Domain::GRID_2D)
            m_diagnosticIdle.beginGrid2DEnterTransition();
        else if (request.domain == Domain::GRID_3D)
            m_diagnosticIdle.beginGrid3DEnterTransition();
        else
            m_diagnosticIdle.beginMulphyEnterTransition();
        return;

    case Request::RETURN_GLOBAL_SHELL:
        m_transitionDomain = request.domain;

        if (request.domain == Domain::GRID_2D) {
            m_domainTransitionPhase = DomainTransitionPhase::EXIT_2D_CAMERA_X;
            if (m_services.camera)
                m_services.camera->beginTransitionToCentered2D(kGrid2DCamLateralDuration);
            return;
        }

        if (request.domain == Domain::GRID_3D) {
            m_domainTransitionPhase = DomainTransitionPhase::EXIT_CAMERA;
            if (m_services.camera)
                m_services.camera->beginTransitionToMenu(kGrid3DCamTransDuration);
            else {
                m_domainTransitionPhase = DomainTransitionPhase::EXIT_DOMAIN_VISUAL;
                m_diagnosticIdle.beginGrid3DReturnTransition();
            }
            return;
        }

        if (request.domain == Domain::MULPHY_SIM) {
            m_domainTransitionPhase = DomainTransitionPhase::EXIT_CAMERA;
            if (m_services.camera)
                m_services.camera->beginTransitionToMenu(kGrid3DCamTransDuration);
            else {
                m_domainTransitionPhase = DomainTransitionPhase::EXIT_DOMAIN_VISUAL;
                m_diagnosticIdle.beginMulphyReturnTransition();
            }
            return;
        }
        return;

    case Request::NONE:
    default:
        return;
    }
}

void Tesseract::updateDomainTransition(const WorkspaceFrameContext& frame) {
    if (!m_services.arbiter || !domainTransitionActive()) return;

    using Domain = TheArbiter::WorkspaceDomain;
    using Layer = TheArbiter::ApplicationLayer;
    using Phase = DomainTransitionPhase;

    switch (m_domainTransitionPhase) {
    case Phase::ENTER_DOMAIN_VISUAL:
        m_diagnosticIdle.update(frame, m_services);

        if (m_transitionDomain == Domain::GRID_2D) {
            if (!m_diagnosticIdle.grid2DEnterVisualComplete()) return;
        }
        else if (m_transitionDomain == Domain::GRID_3D) {
            if (!m_diagnosticIdle.grid3DEnterVisualComplete()) return;
        }
        else if (!m_diagnosticIdle.mulphyEnterVisualComplete()) return;

        m_domainTransitionPhase = Phase::ENTER_DOMAIN_READY;
        m_transitionPhaseElapsed = 0.0f;
        return;

    case Phase::ENTER_DOMAIN_READY:
        m_diagnosticIdle.update(frame, m_services);
        m_transitionPhaseElapsed += frame.deltaTime;
        if (m_transitionPhaseElapsed < kGridReadyHoldDuration) return;

        m_transitionPhaseElapsed = 0.0f;

        if (m_transitionDomain == Domain::GRID_2D) {
            m_domainTransitionPhase = Phase::ENTER_2D_CAMERA_Z;
            if (m_services.camera)
                m_services.camera->beginTransitionToCentered2D(kGrid2DCamCenterDuration);
        }
        else {
            m_domainTransitionPhase = Phase::ENTER_CAMERA;
            if (m_services.camera)
                m_services.camera->beginTransitionToStandard3D(kGrid3DCamTransDuration);
        }
        return;

    case Phase::ENTER_CAMERA:
        m_diagnosticIdle.update(frame, m_services);
        if (m_services.camera) {
            m_services.camera->updatePoseTransition(frame.deltaTime);
            if (m_services.camera->poseTransitionActive()) return;
            m_services.camera->setBehaviorMode(CameraProcessor::CAM_STANDARD_3D);
        }

        m_services.arbiter->setActiveWorkspace(
            m_transitionDomain == Domain::MULPHY_SIM
            ? TheArbiter::WorkspaceId::PARTICLE_SIMULATION
            : TheArbiter::WorkspaceId::GRAPH_3D);
        m_services.arbiter->setApplicationLayer(Layer::DOMAIN_SELECTION);
        m_domainTransitionPhase = Phase::NONE;
        m_transitionDomain = Domain::NONE;
        synchronizeActiveCartridge();
        return;

    case Phase::ENTER_2D_CAMERA_Z:
        m_diagnosticIdle.update(frame, m_services);
        if (m_services.camera) {
            m_services.camera->updatePoseTransition(frame.deltaTime);
            if (m_services.camera->poseTransitionActive()) return;
            m_services.camera->beginTransitionToStandard2D(kGrid2DCamLateralDuration);
        }
        m_domainTransitionPhase = Phase::ENTER_2D_CAMERA_X;
        return;

    case Phase::ENTER_2D_CAMERA_X:
        m_diagnosticIdle.update(frame, m_services);
        if (m_services.camera) {
            m_services.camera->updatePoseTransition(frame.deltaTime);
            if (m_services.camera->poseTransitionActive()) return;
            m_services.camera->setBehaviorMode(CameraProcessor::CAM_STANDARD_2D);
        }

        m_services.arbiter->setActiveWorkspace(
            TheArbiter::WorkspaceId::GRAPH_2D);
        m_services.arbiter->setApplicationLayer(Layer::DOMAIN_SELECTION);
        m_domainTransitionPhase = Phase::NONE;
        m_transitionDomain = Domain::NONE;
        synchronizeActiveCartridge();
        return;

    case Phase::EXIT_CAMERA:
        if (m_activeWorkspace)
            m_activeWorkspace->update(frame, m_services);
        if (m_services.camera) {
            m_services.camera->updatePoseTransition(frame.deltaTime);
            if (m_services.camera->poseTransitionActive()) return;
        }

        m_domainTransitionPhase = Phase::EXIT_DOMAIN_VISUAL;
        if (m_transitionDomain == Domain::MULPHY_SIM)
            m_diagnosticIdle.beginMulphyReturnTransition();
        else
            m_diagnosticIdle.beginGrid3DReturnTransition();
        return;

    case Phase::EXIT_2D_CAMERA_X:
        if (m_activeWorkspace)
            m_activeWorkspace->update(frame, m_services);
        if (m_services.camera) {
            m_services.camera->updatePoseTransition(frame.deltaTime);
            if (m_services.camera->poseTransitionActive()) return;
            m_services.camera->beginTransitionToPreMenu2D(kGrid2DCamCenterDuration);
        }
        m_domainTransitionPhase = Phase::EXIT_2D_CAMERA_Z;
        return;

    case Phase::EXIT_2D_CAMERA_Z:
        if (m_activeWorkspace)
            m_activeWorkspace->update(frame, m_services);
        if (m_services.camera) {
            m_services.camera->updatePoseTransition(frame.deltaTime);
            if (m_services.camera->poseTransitionActive()) return;
        }

        m_domainTransitionPhase = Phase::EXIT_DOMAIN_VISUAL;
        m_diagnosticIdle.beginGrid2DReturnTransition();
        return;

    case Phase::EXIT_DOMAIN_VISUAL:
        m_diagnosticIdle.update(frame, m_services);

        if (m_transitionDomain == Domain::GRID_2D) {
            if (!m_diagnosticIdle.grid2DReturnVisualComplete()) return;
        }
        else if (m_transitionDomain == Domain::GRID_3D) {
            if (!m_diagnosticIdle.grid3DReturnVisualComplete()) return;
        }
        else if (!m_diagnosticIdle.mulphyReturnVisualComplete()) return;

        m_services.arbiter->setApplicationLayer(Layer::GLOBAL_SHELL);
        m_services.arbiter->setActiveWorkspace(TheArbiter::WorkspaceId::DIAGNOSTIC);

        if (m_services.camera)
            m_services.camera->setBehaviorMode(CameraProcessor::CAM_MENU_PREVIEW);

        m_domainTransitionPhase = Phase::NONE;
        m_transitionDomain = Domain::NONE;
        synchronizeActiveCartridge();
        return;

    case Phase::NONE:
    default:
        return;
    }
}

void Tesseract::synchronizeActiveCartridge() {
    if (!m_services.arbiter) return;

    IWorkspace* desired = &m_diagnosticIdle;
    const char* desiredName = "DIAGNOSTIC_IDLE";

    if (!m_services.arbiter->isGlobalShell()) {
        using Domain = TheArbiter::WorkspaceDomain;
        using Workspace = TheArbiter::WorkspaceId;

        if (m_services.arbiter->getWorkspaceDomain() == Domain::GRID_2D) {
            switch (m_services.arbiter->getActiveWorkspace()) {
            case Workspace::HEAT_2D:
                desired = &m_heat2DWorkspace;
                desiredName = "HEAT_2D";
                break;
            case Workspace::GRAPH_2D:
            default:
                desired = &m_graph2DWorkspace;
                desiredName = "GRAPH_2D";
                break;
            }
        }
        else if (m_services.arbiter->getWorkspaceDomain() == Domain::GRID_3D) {
            switch (m_services.arbiter->getActiveWorkspace()) {
            case Workspace::ANN_DESIGN:
                desired = &m_annDesignWorkspace;
                desiredName = "ANN_DESIGN";
                break;
            case Workspace::GRAPH_3D:
            default:
                desired = &m_graph3DWorkspace;
                desiredName = "GRAPH_3D";
                break;
            }
        }
        else if (m_services.arbiter->getWorkspaceDomain() == Domain::MULPHY_SIM) {
            switch (m_services.arbiter->getActiveWorkspace()) {
            case Workspace::MULTIPHYSICS_SIM:
                desired = &m_multiphysicsPlaceholderWorkspace;
                desiredName = "MULTIPHYSICS_SIM";
                break;
            case Workspace::PARTICLE_SIMULATION:
            default:
                desired = &m_particleSimWorkspace;
                desiredName = "PARTICLE_SIMULATION";
                break;
            }
        }
    }

    activateCartridge(desired, desiredName);
}

void Tesseract::activateCartridge(IWorkspace* workspace, const char* name) {
    if (!workspace || workspace == m_activeWorkspace) return;

    if (m_activeWorkspace)
        m_activeWorkspace->exit(m_services);

    m_activeWorkspace = workspace;
    m_activeWorkspace->enter(m_services);

    std::printf("[Tesseract] Active workspace: %s\n",
        name ? name : "UNKNOWN");
}
