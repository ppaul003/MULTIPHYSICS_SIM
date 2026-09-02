#include "rendererEM_Euclid.h"
#include "ANNDesignWorkspace.h"

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

using namespace std;
using namespace glm;

using namespace std;

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

ANNDesignWorkspace::ANNDesignWorkspace() = default;
ANNDesignWorkspace::~ANNDesignWorkspace() {
}

bool ANNDesignWorkspace::initialize(WorkspaceServices & services) {
    (void)services;
    return true;
}

void ANNDesignWorkspace::enter(WorkspaceServices & services) {
    (void)services;
}

void ANNDesignWorkspace::exit(WorkspaceServices & services) {
    (void)services;
}

void ANNDesignWorkspace::update(const WorkspaceFrameContext & frame, WorkspaceServices & services) {

    (void)frame;
    (void)services;
}

void ANNDesignWorkspace::render(const WorkspaceFrameContext & frame, WorkspaceServices & services) {

    (void)frame;
    if (!services.renderer) return;

    EuclidRenderer& renderer = *services.renderer;
    const int gridDim = renderer.getGridDimSize();
    const float boxSize = static_cast<float>(renderer.getSimBoxSize());

    if (gridDim <= 0 || boxSize <= 0.0f) return;

    const float halfBox = boxSize * 0.5f;
    const float cellSize = boxSize / static_cast<float>(gridDim);


    renderer.setGrid(
        ivec3(
            gridDim,
            gridDim,
            gridDim
        ),
        vec3(
            -halfBox,
            -halfBox,
            -halfBox
        ),
        vec3(
            cellSize,
            cellSize,
            cellSize
        )
    );

    renderer.setGridMode3D();
    renderer.setGridStyle(renderer.getGridMajorEvery(), false);

    renderer.setWorkspaceGridVisibility(
        true,   // boundary
        true,   // major grid
        false,  // minor grid
        true    // axes
    );

    renderer.displayGrid();
}

bool ANNDesignWorkspace::handleInput(const WorkspaceInputEvent & input, WorkspaceServices & services) {
    if (!services.arbiter) return false;

    using Workspace = TheArbiter::WorkspaceId;

    switch (input.action) {

        // -----------------------------------------------------
        // GRAPH_3D <- ANN_DESIGN
        // -----------------------------------------------------
    case WorkspaceInputAction::Decrease:
        services.arbiter->setActiveWorkspace(Workspace::GRAPH_3D);
        return true;

        // -----------------------------------------------------
        // GRAPH_3D -> ANN_DESIGN
        // -----------------------------------------------------
    case WorkspaceInputAction::Increase:
        services.arbiter->setActiveWorkspace(Workspace::GRAPH_3D);
        return true;


        // Only one row exists.
    case WorkspaceInputAction::Previous:
    case WorkspaceInputAction::Next:
    case WorkspaceInputAction::Activate:
        return true;


    case WorkspaceInputAction::Back:
        services.arbiter->requestReturnToGlobalShell(TheArbiter::WorkspaceDomain::GRID_3D);
        return true;


    default:
        return false;
    }
}

WorkspacePresentation ANNDesignWorkspace::buildPresentation() const {
    return buildLayer1Presentation();
}

WorkspacePresentation
ANNDesignWorkspace::buildLayer1TransitionPresentation() const {
    return buildLayer1Presentation();
}

WorkspacePresentation ANNDesignWorkspace::buildLayer1Presentation() const {

    WorkspacePresentation p;
    p.panelVisible = true;
    p.workspaceName = "LAYER 1 -> GRID_3D WORKSPACE CONFIGURATION";
    p.layerLabel = "MODE: ANN_DESIGN";

    WorkspacePanelSection section;
    section.rows.push_back(
        makeRow(
            "[1]: GRID_3D SELECTION",
            "ANN_DESIGN",
            true
        )
    );

    p.sections.push_back(section);
    p.statusLine = "ANN_DESIGN is reserved for the next pass.";
    p.statusTone = WorkspaceStatusTone::Warning;
    p.footerLine1 = "W / S: Select list     A / D: Change value";
    p.footerLine2 = "E: Activate selected row     Q: Back one layer";

    return p;

}
