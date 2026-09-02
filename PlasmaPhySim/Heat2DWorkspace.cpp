#include "rendererEM_Euclid.h"
#include "Heat2DWorkspace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

using namespace std;
using namespace glm;

namespace {
    WorkspacePanelRow makeRow(
        const string& label,
        const string& value,
        bool selected,
        bool selectable = true) {

        WorkspacePanelRow row;
        row.label = label;
        row.value = value;
        row.selectable = selectable;
        row.selected = selected;
        return row;
    }

    template <typename Enum>
    Enum cycleEnum(Enum value, int count, int direction) {
        const int step = direction < 0 ? -1 : 1;
        const int current = static_cast<int>(value);
        return static_cast<Enum>((current + step + count) % count);
    }
}

Heat2DWorkspace::Heat2DWorkspace() = default;
Heat2DWorkspace::~Heat2DWorkspace() {
}

bool Heat2DWorkspace::initialize(WorkspaceServices & services) {
    (void)services;
    return true;
}

void Heat2DWorkspace::enter(WorkspaceServices & services) {
    (void)services;
}

void Heat2DWorkspace::exit(WorkspaceServices & services) {
    (void)services;
}

void Heat2DWorkspace::update(
    const WorkspaceFrameContext & frame,
    WorkspaceServices & services) {

    (void)frame;
    (void)services;
}

void Heat2DWorkspace::render(
    const WorkspaceFrameContext & frame,
    WorkspaceServices & services) {

    (void)frame;

    if (!services.renderer) return;

    EuclidRenderer& renderer = *services.renderer;
    EuclidRenderer::UniformGrid grid;

    const float boxSize =
        static_cast<float>(renderer.getSimBoxSize());

    const float halfBox = boxSize * 0.5f;

    grid.dimensions = ivec3(64);
    grid.origin = vec3(-halfBox);
    grid.cellSize = vec3(boxSize / 64.0f);
    grid.majorEvery = 8;

    // =====================================================
    // Stable GRID_2D domain visual
    //
    // Collapsed XY plane held at the +Z/front face.
    // No 3D lattice. No cube.
    // =====================================================
    const float planeZ = grid.origin.z + boxSize;

    renderer.drawGridPlane(
        grid,
        EuclidRenderer::GridPlane::PLANE_XY,
        planeZ,
        false
    );
}

bool Heat2DWorkspace::handleInput(
    const WorkspaceInputEvent & input,
    WorkspaceServices & services) {

    if (!services.arbiter)
        return false;

    switch (input.action) {

    case WorkspaceInputAction::Decrease:
    case WorkspaceInputAction::Increase:
        services.arbiter->setActiveWorkspace(TheArbiter::WorkspaceId::GRAPH_2D);
        return true;


    case WorkspaceInputAction::Back:
        services.arbiter->requestReturnToGlobalShell(TheArbiter::WorkspaceDomain::GRID_2D);

        return true;

    default:
        return false;
    }
}

WorkspacePresentation Heat2DWorkspace::buildPresentation() const {
    return buildLayer1Presentation();
}

WorkspacePresentation
Heat2DWorkspace::buildLayer1TransitionPresentation() const {
    return buildLayer1Presentation();
}

WorkspacePresentation Heat2DWorkspace::buildLayer1Presentation() const {

    WorkspacePresentation p;
    p.panelVisible = true;

    p.workspaceName = "LAYER 1 -> GRID_2D WORKSPACE CONFIGURATION";
    p.layerLabel = "MODE: HEAT_2D";

    WorkspacePanelSection section;
    section.rows.push_back(
        makeRow(
            "[1]: GRID_2D SELECTION",
            "HEAT_2D",
            true
        )
    );

    p.sections.push_back(section);
    p.statusLine = "HEAT_2D is reserved for the next pass.";
    p.statusTone = WorkspaceStatusTone::Warning;
    p.footerLine1 = "W / S: Select list     A / D: Change value";
    p.footerLine2 = "E: Activate selected row     Q: Back one layer";

    return p;

}
