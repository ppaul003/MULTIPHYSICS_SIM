#include "multiPhysicsSimWorkspace.h"
#include "IWorkspaceEM.h"
#include "rendererEM_Euclid.h"

#include <algorithm>
#include <cmath>

using namespace std;
using namespace glm;

bool MultiPhysicsSimWorkspace::initialize(WorkspaceServices& services) {
    if (!services.arbiter || !services.renderer) return false;

    m_arbiter = services.arbiter;
    return true;
}

void MultiPhysicsSimWorkspace::enter(WorkspaceServices& services) {
    if (!m_arbiter) m_arbiter = services.arbiter;

    m_active = true;
}

void MultiPhysicsSimWorkspace::exit(WorkspaceServices& services) {
    (void)services;
    m_active = false;
}

void MultiPhysicsSimWorkspace::update(
    const WorkspaceFrameContext& frame,
    WorkspaceServices& services) {

    // Reserved workspace: no physics yet.
    (void)frame;
    (void)services;
}

void MultiPhysicsSimWorkspace::render(
    const WorkspaceFrameContext& frame,
    WorkspaceServices& services) {

    if (!frame.displayEnabled ||
        !services.renderer)
        return;

    if (services.arbiter &&
        services.arbiter->isDomainSelection()) {

        renderLayer1DomainBoundary(services);
        return;
    }
}

bool MultiPhysicsSimWorkspace::handleInput(
    const WorkspaceInputEvent& input,
    WorkspaceServices& services) {

    if (!m_active ||!services.arbiter) return false;

    using Workspace = TheArbiter::WorkspaceId;

    switch (input.action) {

    case WorkspaceInputAction::Decrease:
    case WorkspaceInputAction::Increase:

        services.arbiter->setActiveWorkspace(
            Workspace::PARTICLE_SIMULATION
        );

        return true;

    case WorkspaceInputAction::Previous:
    case WorkspaceInputAction::Next:

        // Only one row exists.
        return true;

    case WorkspaceInputAction::Activate:

        // Layer 2 is not implemented yet.
        return true;

    case WorkspaceInputAction::Back:

        services.arbiter->requestReturnToGlobalShell(
                TheArbiter::WorkspaceDomain::MULPHY_SIM
            );

        return true;

    default:
        return false;
    }
}

void MultiPhysicsSimWorkspace::renderLayer1DomainBoundary(WorkspaceServices& services) const {
    if (!services.renderer) return;

    EuclidRenderer& renderer = *services.renderer;

    const float boxSize = static_cast<float>(renderer.getSimBoxSize());
    const float halfBox = boxSize * 0.5f;
    const int gridDim = std::max(1, renderer.getGridDimSize());

    EuclidRenderer::UniformGrid grid;

    grid.dimensions = ivec3(gridDim, gridDim, gridDim);
    grid.origin = vec3(-halfBox, -halfBox, -halfBox);
    grid.cellSize = vec3(boxSize / static_cast<float>(gridDim));

    EuclidRenderer::GridDisplay display;

    display.boundary = true;
    display.majorGrid = false;
    display.minorGrid = false;
    display.axes = false;

    renderer.drawUniformGrid(grid, display);
}

WorkspacePresentation
MultiPhysicsSimWorkspace::buildLayer1Presentation() const {

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

    p.sections.push_back(section);

    p.statusLine = "MULTIPHYSICS_SIM is reserved for the next pass.";
    p.statusTone =  WorkspaceStatusTone::Warning;

    p.footerLine1 = "A / D: Change workspace";
    p.footerLine2 = "Q: Return to Global Shell    ESC: Exit";

    return p;
}

WorkspacePresentation
MultiPhysicsSimWorkspace::buildPresentation() const {

    return buildLayer1Presentation();
}

WorkspacePresentation
MultiPhysicsSimWorkspace::buildLayer1TransitionPresentation() const {

    return buildLayer1Presentation();
}