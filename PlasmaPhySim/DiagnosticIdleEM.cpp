#include "DiagnosticIdleEM.h"
#include "rendererEM_Euclid.h"
#include "TheArbiterEM.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace glm;

bool DiagnosticIdle::initialize(WorkspaceServices& services) {
    m_arbiter = services.arbiter;
    return services.renderer != nullptr && m_arbiter != nullptr;
}

void DiagnosticIdle::enter(WorkspaceServices& services) {
    (void)services;
}

void DiagnosticIdle::exit(WorkspaceServices& services) {
    (void)services;
}

void DiagnosticIdle::update(
    const WorkspaceFrameContext& frame,
    WorkspaceServices& services) {

    (void)services;
    const float dt = frame.deltaTime;

    switch (m_visualTransition) {
    case VisualTransitionState::Idle:
        m_previewRotationDegrees += kPreviewRotationSpeed * dt;
        m_sliceTravel += kSliceCycleSpeed * dt;
        break;

    case VisualTransitionState::Grid3D_OrientToFront:
        m_previewRotationDegrees += kTransitionRotationSpeed * dt;
        m_sliceTravel += kTransitionSliceSpeed * dt;

        if (m_previewRotationDegrees >= m_targetRotationDegrees) {
            m_previewRotationDegrees = m_targetRotationDegrees;

            const float cycle = std::floor(m_sliceTravel / 3.0f);
            float target = cycle * 3.0f + 0.5f;
            if (target <= m_sliceTravel) target += 3.0f;

            m_targetSliceTravel = target;
            m_visualTransition = VisualTransitionState::Grid3D_CaptureSlice;
        }
        break;

    case VisualTransitionState::Grid3D_CaptureSlice:
        m_sliceTravel += kTransitionSliceSpeed * dt;
        if (m_sliceTravel >= m_targetSliceTravel) {
            m_sliceTravel = m_targetSliceTravel;
            m_visualTransition = VisualTransitionState::Grid3D_HoldCenter;
            m_grid3DEnterComplete = true;
        }
        break;

    case VisualTransitionState::Grid3D_HoldCenter:
        break;

    case VisualTransitionState::Grid3D_ReleaseSlice:
        m_sliceTravel += kSliceCycleSpeed * dt;
        if (m_sliceTravel >= m_targetSliceTravel) {
            m_sliceTravel = m_targetSliceTravel;
            m_visualTransition = VisualTransitionState::Idle;
            m_grid3DReturnComplete = true;
        }
        break;

    case VisualTransitionState::Grid2D_OrientToFront:
        m_previewRotationDegrees += kTransitionRotationSpeed * dt;
        m_sliceTravel += kTransitionSliceSpeed * dt;
        if (m_previewRotationDegrees >= m_targetRotationDegrees) {
            m_previewRotationDegrees = m_targetRotationDegrees;

            float target = std::floor(m_sliceTravel / 3.0f) * 3.0f;
            if (target <= m_sliceTravel) target += 3.0f;

            m_targetSliceTravel = target;
            m_visualTransition = VisualTransitionState::Grid2D_WaitForXYStart;
        }
        break;

    case VisualTransitionState::Grid2D_WaitForXYStart:
        m_sliceTravel += kTransitionSliceSpeed * dt;
        if (m_sliceTravel >= m_targetSliceTravel) {
            m_sliceTravel = m_targetSliceTravel;
            m_grid2DPlaneProgress = 0.0f;
            m_visualTransition = VisualTransitionState::Grid2D_SweepToFront;
        }
        break;

    case VisualTransitionState::Grid2D_SweepToFront:
        m_grid2DPlaneProgress = std::min(
            1.0f,
            m_grid2DPlaneProgress + kTransitionSliceSpeed * dt);
        m_sliceTravel = m_targetSliceTravel + m_grid2DPlaneProgress;

        if (m_grid2DPlaneProgress >= 1.0f) {
            m_visualTransition = VisualTransitionState::Grid2D_HoldFront;
            m_grid2DEnterComplete = true;
        }
        break;

    case VisualTransitionState::Grid2D_HoldFront:
        break;

    case VisualTransitionState::Grid2D_ReturnSweep:
        m_grid2DPlaneProgress = std::max(
            0.0f,
            m_grid2DPlaneProgress - kTransitionSliceSpeed * dt);
        m_sliceTravel = m_targetSliceTravel + m_grid2DPlaneProgress;

        if (m_grid2DPlaneProgress <= 0.0f) {
            m_sliceTravel = m_targetSliceTravel;
            m_visualTransition = VisualTransitionState::Idle;
            m_grid2DReturnComplete = true;
        }
        break;
    }
}

void DiagnosticIdle::render(
    const WorkspaceFrameContext& frame,
    WorkspaceServices& services) {

    (void)frame;
    if (!services.renderer) return;

    EuclidRenderer& renderer = *services.renderer;
    EuclidRenderer::UniformGrid grid;

    constexpr int kGridDim = 64;
    constexpr int kMajorEvery = 8;

    const float boxSize = static_cast<float>(renderer.getSimBoxSize());
    const float halfBox = boxSize * 0.5f;
    const float cellSize = boxSize / static_cast<float>(kGridDim);

    grid.dimensions = ivec3(kGridDim);
    grid.origin = vec3(-halfBox);
    grid.cellSize = vec3(cellSize);
    grid.majorEvery = kMajorEvery;

    EuclidRenderer::GridDisplay display;
    display.boundary = true;
    display.majorGrid = true;
    display.minorGrid = false;
    display.axes = true;

    glPushMatrix();

    const float visualRotation = std::fmod(m_previewRotationDegrees, 360.0f);
    glRotatef(visualRotation, 0.0f, 1.0f, 0.0f);

    const bool grid2DPlanarVisual =
        m_visualTransition == VisualTransitionState::Grid2D_SweepToFront ||
        m_visualTransition == VisualTransitionState::Grid2D_HoldFront ||
        m_visualTransition == VisualTransitionState::Grid2D_ReturnSweep;

    if (grid2DPlanarVisual) {
        const float planePosition =
            grid.origin.z + boxSize * m_grid2DPlaneProgress;

        renderer.drawUniformGridZRange(
            grid,
            planePosition,
            grid.origin.z + boxSize,
            display);

        renderer.drawGridPlane(
            grid,
            EuclidRenderer::PLANE_XY,
            planePosition,
            false);

        glPopMatrix();
        return;
    }

    renderer.drawUniformGrid(grid, display);

    float sliceCycle = std::fmod(m_sliceTravel, 3.0f);
    if (sliceCycle < 0.0f) sliceCycle += 3.0f;

    const int segment = std::min(2, static_cast<int>(sliceCycle));
    const float local = sliceCycle - static_cast<float>(segment);

    const int halfSlice = kGridDim / 2;
    const int sliceOffset = static_cast<int>(std::round(
        -halfSlice + local * static_cast<float>(halfSlice * 2)));
    const int sliceIndex = std::clamp(halfSlice + sliceOffset, 0, kGridDim);

    EuclidRenderer::GridPlane plane = EuclidRenderer::PLANE_XY;
    float planePosition = 0.0f;

    switch (segment) {
    case 0:
        plane = EuclidRenderer::PLANE_XY;
        planePosition = grid.origin.z +
            static_cast<float>(sliceIndex) * grid.cellSize.z;
        break;
    case 1:
        plane = EuclidRenderer::PLANE_XZ;
        planePosition = grid.origin.y +
            static_cast<float>(sliceIndex) * grid.cellSize.y;
        break;
    default:
        plane = EuclidRenderer::PLANE_YZ;
        planePosition = grid.origin.x +
            static_cast<float>(sliceIndex) * grid.cellSize.x;
        break;
    }

    renderer.drawGridPlane(grid, plane, planePosition, false);
    glPopMatrix();
}

bool DiagnosticIdle::handleInput(
    const WorkspaceInputEvent& input,
    WorkspaceServices& services) {

    if (!m_arbiter) m_arbiter = services.arbiter;
    if (!m_arbiter) return false;

    if (m_arbiter->isGlobalShell()) {
        switch (input.action) {
        case WorkspaceInputAction::Previous:
            moveGlobalShellCursor(-1);
            return true;
        case WorkspaceInputAction::Next:
            moveGlobalShellCursor(+1);
            return true;
        case WorkspaceInputAction::Decrease:
            adjustGlobalShellValue(-1);
            return true;
        case WorkspaceInputAction::Increase:
            adjustGlobalShellValue(+1);
            return true;
        case WorkspaceInputAction::Activate:
            if (m_activeShellRow != GlobalShellRow::Configure)
                return true;
            if (m_arbiter->getWorkspaceDomain() == TheArbiter::WorkspaceDomain::NONE)
                return true;
            if (m_arbiter->getUnitMeasurement() ==
                TheArbiter::UnitMeasurement::IMPERIAL)
                return true;
            if (m_arbiter->getWorkspaceDomain() ==
                TheArbiter::WorkspaceDomain::MULPHY_SIM) {
                m_showMultiphysicsNotMigrated = true;
                return true;
            }
            if (m_requestedSimBoxSize != 4)
                return true;

            if (services.renderer)
                services.renderer->setSimBoxSize(m_requestedSimBoxSize);

            m_showMultiphysicsNotMigrated = false;
            m_arbiter->requestEnterDomain(m_arbiter->getWorkspaceDomain());
            return true;

        case WorkspaceInputAction::Back:
            return true;
        default:
            return false;
        }
    }

    if (m_arbiter->isDomainSelection() &&
        input.action == WorkspaceInputAction::Back) {
        m_arbiter->requestReturnToGlobalShell(m_arbiter->getWorkspaceDomain());
        return true;
    }

    return false;
}

WorkspacePresentation DiagnosticIdle::buildPresentation() const {
    WorkspacePresentation p;
    p.panelVisible = true;

    if (m_arbiter && !m_arbiter->isGlobalShell()) {
        p.workspaceName = std::string("LAYER 1 -> ") + selectedEnvironmentName();
        p.layerLabel = "BASELINE DOMAIN HOLD";
        p.statusLine = "No workspace cartridge loaded. Diagnostic transition endpoint only.";
        p.statusTone = WorkspaceStatusTone::Warning;
        p.footerLine1 = "Q: Return to diagnostic shell";
        p.footerLine2 = "ESC: Exit";
        return p;
    }

    p.workspaceName = "LAYER 0 -> GLOBAL SHELL CONFIG";

    WorkspacePanelSection section;

    WorkspacePanelRow domainRow;
    domainRow.label = "[1]: DOMAIN SELECTION";
    domainRow.value = selectedEnvironmentName();
    domainRow.selectable = true;
    domainRow.selected = m_activeShellRow == GlobalShellRow::Environment;
    section.rows.push_back(domainRow);

    WorkspacePanelRow boxRow;
    boxRow.label = "[2]: SIMULATION BOX SIZE";
    boxRow.value = std::to_string(m_requestedSimBoxSize);
    boxRow.selectable = true;
    boxRow.selected = m_activeShellRow == GlobalShellRow::SimulationBox;
    section.rows.push_back(boxRow);

    WorkspacePanelRow unitRow;
    unitRow.label = "[3]: SIM UNIT MEASUREMENT";
    unitRow.value = selectedUnitMeasurementName();
    unitRow.selectable = true;
    unitRow.selected = m_activeShellRow == GlobalShellRow::SimulationUnit;
    section.rows.push_back(unitRow);

    WorkspacePanelRow configureRow;
    configureRow.label = "[4]: E TO CONFIG GLOBAL SHELL";
    configureRow.selectable = true;
    configureRow.selected = m_activeShellRow == GlobalShellRow::Configure;
    section.rows.push_back(configureRow);

    p.sections.push_back(section);

    if (m_showMultiphysicsNotMigrated) {
        p.statusLine = "MULTIPHYSICS_SIM cartridge is not migrated.";
        p.statusTone = WorkspaceStatusTone::Warning;
    }
    else if (m_requestedSimBoxSize != 4) {
        p.statusLine = "WARNING: SIMULATION BOX SIZE {" +
            std::to_string(m_requestedSimBoxSize) +
            "} IS UNAVAILABLE. SELECT { 4 } TO CONTINUE.";
        p.statusTone = WorkspaceStatusTone::Warning;
    }
    else if (!m_arbiter || m_arbiter->getWorkspaceDomain() == TheArbiter::WorkspaceDomain::NONE) {
        p.statusLine = "IDLE selected: choose a workspace environment.";
        p.statusTone = WorkspaceStatusTone::Warning;
    }
    else {
        p.statusLine = "READY: GLOBAL SHELL CONFIGURATION VALID.";
        p.statusTone = WorkspaceStatusTone::Ready;
    }

    if (m_arbiter && m_arbiter->getUnitMeasurement() == TheArbiter::UnitMeasurement::IMPERIAL) {

        p.statusLine =
            "Unit measurement unavailable.";

        p.statusTone =
            WorkspaceStatusTone::Warning;
    }
    else if (!m_arbiter || m_arbiter->getWorkspaceDomain() == TheArbiter::WorkspaceDomain::NONE) {

        p.statusLine =
            "IDLE selected: choose a workspace environment.";

        p.statusTone =
            WorkspaceStatusTone::Warning;
    }
    else {

        p.statusLine =
            "READY: GLOBAL SHELL CONFIGURATION VALID.";

        p.statusTone =
            WorkspaceStatusTone::Ready;
    }

    p.footerLine1 = "W/S: Select row    A/D: Change value    E: Configure";
    p.footerLine2 = "ESC: Exit";

    return p;
}

void DiagnosticIdle::beginGrid3DEnterTransition() {
    m_grid3DEnterComplete = false;
    m_grid3DReturnComplete = false;

    const float revolution = std::floor(m_previewRotationDegrees / 360.0f);
    m_targetRotationDegrees = (revolution + 1.0f) * 360.0f;
    m_visualTransition = VisualTransitionState::Grid3D_OrientToFront;
}

void DiagnosticIdle::beginGrid3DReturnTransition() {
    m_grid3DReturnComplete = false;
    m_targetSliceTravel = m_sliceTravel + 0.5f;
    m_visualTransition = VisualTransitionState::Grid3D_ReleaseSlice;
}

void DiagnosticIdle::beginGrid2DEnterTransition() {
    m_grid2DEnterComplete = false;
    m_grid2DReturnComplete = false;
    m_grid2DPlaneProgress = 0.0f;

    const float revolution = std::floor(m_previewRotationDegrees / 360.0f);
    m_targetRotationDegrees = (revolution + 1.0f) * 360.0f;
    m_visualTransition = VisualTransitionState::Grid2D_OrientToFront;
}

void DiagnosticIdle::beginGrid2DReturnTransition() {
    m_grid2DReturnComplete = false;
    m_grid2DPlaneProgress = 1.0f;

    m_targetSliceTravel = std::floor(m_sliceTravel / 3.0f) * 3.0f;
    m_sliceTravel = m_targetSliceTravel + 1.0f;
    m_visualTransition = VisualTransitionState::Grid2D_ReturnSweep;
}

const char* DiagnosticIdle::selectedEnvironmentName() const {
    if (!m_arbiter) return "IDLE";

    switch (m_arbiter->getWorkspaceDomain()) {
    case TheArbiter::WorkspaceDomain::GRID_2D: return "GRID_2D";
    case TheArbiter::WorkspaceDomain::GRID_3D: return "GRID_3D";
    case TheArbiter::WorkspaceDomain::MULPHY_SIM: return "MULPHY_SIM";
    case TheArbiter::WorkspaceDomain::NONE:
    default:
        return "IDLE";
    }
}

const char* DiagnosticIdle::selectedUnitMeasurementName() const {
    if (!m_arbiter)
        return "METRIC";

    switch (m_arbiter->getUnitMeasurement()) {

    case TheArbiter::UnitMeasurement::IMPERIAL:
        return "IMPERIAL";

    case TheArbiter::UnitMeasurement::METRIC:
    default:
        return "METRIC";
    }
}

void DiagnosticIdle::cycleEnvironment(int direction) {
    if (!m_arbiter || direction == 0) return;

    using Domain = TheArbiter::WorkspaceDomain;
    static constexpr Domain order[] = {
        Domain::NONE,
        Domain::GRID_2D,
        Domain::GRID_3D,
        Domain::MULPHY_SIM
    };

    int current = 0;
    for (int i = 0; i < 4; i++) {
        if (order[i] == m_arbiter->getWorkspaceDomain()) {
            current = i;
            break;
        }
    }

    const int step = direction < 0 ? -1 : 1;
    const int next = (current + step + 4) % 4;
    m_arbiter->setWorkspaceDomain(order[next]);
    m_showMultiphysicsNotMigrated = false;
}

void DiagnosticIdle::cycleUnitMeasurement(int direction) {
    if (!m_arbiter || direction == 0)
        return;

    using Unit = TheArbiter::UnitMeasurement;

    const Unit current =
        m_arbiter->getUnitMeasurement();

    if (current == Unit::METRIC) {
        m_arbiter->setUnitMeasurement(Unit::IMPERIAL);
    }
    else {
        m_arbiter->setUnitMeasurement(Unit::METRIC);
    }
}

void DiagnosticIdle::moveGlobalShellCursor(int direction) {
    if (direction == 0) return;

    const int count = static_cast<int>(GlobalShellRow::Count);
    const int current = static_cast<int>(m_activeShellRow);
    const int step = direction < 0 ? -1 : 1;

    m_activeShellRow = static_cast<GlobalShellRow>(
        (current + step + count) % count);
}

void DiagnosticIdle::adjustGlobalShellValue(int direction) {
    if (direction == 0) return;

    switch (m_activeShellRow) {
    case GlobalShellRow::Environment:
        cycleEnvironment(direction);
        break;

    case GlobalShellRow::SimulationBox: {
        static constexpr int kBoxSizes[] = { 2, 4, 8, 16 };
        int index = 0;

        for (int i = 0; i < 4; ++i) {
            if (kBoxSizes[i] == m_requestedSimBoxSize) {
                index = i;
                break;
            }
        }

        const int step = direction < 0 ? -1 : 1;
        index = (index + step + 4) % 4;
        m_requestedSimBoxSize = kBoxSizes[index];
        m_showMultiphysicsNotMigrated = false;
        break;
    }

    case GlobalShellRow::SimulationUnit:
        cycleUnitMeasurement(direction);
        break;

    case GlobalShellRow::Configure:
    case GlobalShellRow::Count:
    default:
        break;
    }
}
