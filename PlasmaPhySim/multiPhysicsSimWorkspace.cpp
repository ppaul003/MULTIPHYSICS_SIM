#include "multiPhysicsSimWorkspace.h"

#include "rendererEM_Euclid.h"

#include <string>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace glm;

namespace {

    WorkspacePanelRow makeRow(
        const string& label,
        const string& value,
        bool selected) {

        WorkspacePanelRow row;
        row.label = label;
        row.value = value;
        row.selectable = true;
        row.selected = selected;

        return row;
    }

} // namespace

bool MultiPhysicsSimWorkspace::initialize(WorkspaceServices& services) {
    if (m_initialized) return true;
    if (!services.arbiter) return false;

    m_arbiter = services.arbiter;
    m_initialized = true;

    return true;
}

void MultiPhysicsSimWorkspace::enter(WorkspaceServices& services) {
    if (!m_arbiter) m_arbiter = services.arbiter;
    if (!m_initialized) return;

    m_active = true;

    refreshLayer1Status();
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
        !services.renderer ||
        !services.arbiter) {
        return;
    }

    switch (services.arbiter->getApplicationLayer()) {
    case TheArbiter::ApplicationLayer::DOMAIN_SELECTION:
        renderConfiguredGrid(services, m_draftConfig.gridLayout);
        return;

    case TheArbiter::ApplicationLayer::WORKSPACE_CONFIGURATION:
    case TheArbiter::ApplicationLayer::ACTIVE_WORKSPACE:
    case TheArbiter::ApplicationLayer::GLOBAL_SHELL:
    default:
        return;

    }
}

bool MultiPhysicsSimWorkspace::handleInput(
    const WorkspaceInputEvent& input,
    WorkspaceServices& services) {

    if (!m_active ||!services.arbiter) return false;
    if (!m_arbiter) m_arbiter = services.arbiter;

    switch (services.arbiter->getApplicationLayer()) {
    case TheArbiter::ApplicationLayer::DOMAIN_SELECTION:
        return handleLayer1Input(input, services);

    case TheArbiter::ApplicationLayer::WORKSPACE_CONFIGURATION:
    case TheArbiter::ApplicationLayer::ACTIVE_WORKSPACE:
    case TheArbiter::ApplicationLayer::GLOBAL_SHELL:
    default:
        return false;
    }
}

bool MultiPhysicsSimWorkspace::handleLayer1Input(
    const WorkspaceInputEvent& input,
    WorkspaceServices& services) {

    switch (input.action) {
    case WorkspaceInputAction::Previous:
        moveLayer1Cursor(-1);
        return true;

    case WorkspaceInputAction::Next:
        moveLayer1Cursor(+1);
        return true;
        
    case WorkspaceInputAction::Decrease:
        adjustLayer1Value(-1, services);
        return true;

    case WorkspaceInputAction::Increase:
        adjustLayer1Value(+1, services);
        return true;

    case WorkspaceInputAction::Activate:
        if (m_layer1Selection == Layer1Row::WorkspaceSelection) {
            adjustLayer1Value(+1, services);
        }
        else if (m_layer1Selection == Layer1Row::Configure) {
            if (m_draftConfig.gridLayout == GridLayout::Dynamic) {
                m_statusLine = "DYNAMIC GRID is unavailable for this pass.";
                m_statusTone = WorkspaceStatusTone::Warning;
            }
            else {
                m_statusLine = "LAYER 2 CONFIGURATION is unavailable in current pass";
                m_statusTone = WorkspaceStatusTone::Warning;
            }
        }
        return true;

    case WorkspaceInputAction::Back:
        services.arbiter->requestReturnToGlobalShell(
            TheArbiter::WorkspaceDomain::MULPHY_SIM
        );
        return true;

    case WorkspaceInputAction::RawKey:
    case WorkspaceInputAction::None:
    default:
        return false;
    }
}

void MultiPhysicsSimWorkspace::renderConfiguredGrid(
    WorkspaceServices& services, GridLayout layout) const {

    if (!services.renderer) return;

    EuclidRenderer::UniformGrid grid;

    grid.dimensions = ivec3(
        static_cast<int>(kGridSize),
        static_cast<int>(kGridSize),
        static_cast<int>(kGridSize)
    );

    grid.origin = m_baseVoxelGrid.origin;
    grid.cellSize = vec3(kCellSizeM, kCellSizeM, kCellSizeM);
    grid.majorEvery = static_cast<int>(kMajorGridEvery);

    EuclidRenderer::GridDisplay display;
    display.boundary = true;
    display.majorGrid = layout == GridLayout::MajorGrid;
    display.minorGrid = false;
    display.axes = false;

    services.renderer->drawUniformGrid(grid, display);
}

WorkspacePresentation
MultiPhysicsSimWorkspace::buildLayer1Presentation() const {

    WorkspacePresentation p;

    p.panelVisible = true;
    p.workspaceName = "LAYER 1 -> MULPHY_SIM WORKSPACE CONFIGURATION";
    p.layerLabel = "MODE: MULTIPHYSICS_SIM";

    WorkspacePanelSection section;

    section.rows.push_back(makeRow(
        "[1]: MULPHY_SIM SELECTION",
        "MULTIPHYSICS_SIM",
        m_layer1Selection == Layer1Row::WorkspaceSelection
    ));

    section.rows.push_back(makeRow(
        "[2]: GRID LAYOUT",
        gridLayoutName(),
        m_layer1Selection == Layer1Row::GridLayout
    ));

    section.rows.push_back(makeRow(
        "[3]: MULTIPHYSICS MODE",
        multiphysicsModeName(),
        m_layer1Selection == Layer1Row::MultiphysicsMode
    ));

    section.rows.push_back(makeRow(
        "[4]: SIM SPACE MEDIUM",
        simSpaceMediumName(),
        m_layer1Selection == Layer1Row::SimSpaceMedium
    ));

    section.rows.push_back(makeRow(
        "[5]: PRESS E TO CONFIGURE WORKSPACE",
        "",
        m_layer1Selection == Layer1Row::Configure
    ));

    p.sections.push_back(section);
    
    p.statusLine = m_statusLine;
    p.statusTone = m_statusTone;
    p.footerLine1 = "W/S: Select row    A/D: Change value    E: Configure";
    p.footerLine2 = "Q: Return to Global Shell    ESC: Exit";
    return p;
}

WorkspacePresentation MultiPhysicsSimWorkspace::buildPresentation() const {
    if (!m_arbiter) return {};

    switch (m_arbiter->getApplicationLayer()) {
    case TheArbiter::ApplicationLayer::DOMAIN_SELECTION:
        return buildLayer1Presentation();

    case TheArbiter::ApplicationLayer::WORKSPACE_CONFIGURATION:
    case TheArbiter::ApplicationLayer::ACTIVE_WORKSPACE:
    case TheArbiter::ApplicationLayer::GLOBAL_SHELL:
    default:
        return {};

    }
}

void MultiPhysicsSimWorkspace::moveLayer1Cursor(int direction) {
    if (direction == 0) return;
    const int count = static_cast<int>(Layer1Row::Count);
    const int current = static_cast<int>(m_layer1Selection);
    const int step = direction < 0 ? -1 : 1;
    m_layer1Selection = static_cast<Layer1Row>(
        (current + step + count) % count
    );
}

void MultiPhysicsSimWorkspace::adjustLayer1Value(
    int direction, 
    WorkspaceServices& services) {

    if (direction == 0) return;

    const int step = direction < 0 ? -1 : +1;

    switch (m_layer1Selection) {
    // MULTIPHYSICS_SIM <-> PARTICLE_SIM
    //
    // This preserves the same cartridge-selection
    // behavior PARTICLE_SIM already uses.
    case Layer1Row::WorkspaceSelection:
        if (services.arbiter) {
            services.arbiter->setActiveWorkspace(
                TheArbiter::WorkspaceId::PARTICLE_SIMULATION
            );
        }
        return;

    case Layer1Row::GridLayout: {
        const int count = static_cast<int>(GridLayout::Count);
        const int current = static_cast<int>(m_draftConfig.gridLayout);
        m_draftConfig.gridLayout = static_cast<GridLayout>(
            (current + step + count) % count
        );
        break;
    }
    case Layer1Row::MultiphysicsMode: {
        const int count = static_cast<int>(MultiphysicsMode::Count);
        const int current = static_cast<int>(m_draftConfig.multphysicsMode);
        m_draftConfig.multphysicsMode = static_cast<MultiphysicsMode>(
            (current + step + count) % count
        );
        break;
    }
    case Layer1Row::SimSpaceMedium: {
        const int count = static_cast<int>(SimSpaceMedium::Count);
        const int current = static_cast<int>(m_draftConfig.simSpaceMedium);
        m_draftConfig.simSpaceMedium = static_cast<SimSpaceMedium>(
            (current + step + count) % count
        );
        break;
    }
    case Layer1Row::Configure:
    case Layer1Row::Count:
    default:
        return;
    }
}

void MultiPhysicsSimWorkspace::refreshLayer1Status() {
    // Highest-priority warning:
    // DYNAMIC GRID is not available.
    if (m_draftConfig.gridLayout == GridLayout::Dynamic) {
        
        m_statusLine = "DYNAMIC GRID is unavailable for this pass.";
        m_statusTone = WorkspaceStatusTone::Warning;

        return;
    }

    switch (m_draftConfig.multphysicsMode) {

    case MultiphysicsMode::Electrodynamics:
        m_statusLine = "ELECTRODYNAMICS reserved for future pass.";
        m_statusTone = WorkspaceStatusTone::Warning;
        return;

    case MultiphysicsMode::EmWave:
        m_statusLine = "EM_WAVE reserved for future pass.";
        m_statusTone = WorkspaceStatusTone::Warning;
        return;

    case MultiphysicsMode::PlasmaPhy:
    default:
        break;
    }

    m_statusLine = "Ready: MULPHY_SIM WORKSPACE.";
    m_statusTone = WorkspaceStatusTone::Ready;
}

const char* MultiPhysicsSimWorkspace::gridLayoutName() const {
    switch (m_draftConfig.gridLayout) {
    case GridLayout::MajorGrid: return "MAJOR_GRID";
    case GridLayout::Dynamic: return "DYNAMIC";
    case GridLayout::None:
    default:
        return "NONE";
    }
}

const char* MultiPhysicsSimWorkspace::multiphysicsModeName() const {
    switch (m_draftConfig.multphysicsMode) {
    case MultiphysicsMode::EmWave: return "EM_WAVE";
    case MultiphysicsMode::PlasmaPhy: return "PLASMA_PHYSICS";
    case MultiphysicsMode::Electrodynamics: return "ELECTRODYNAMICS";
    default:
        return "PLASMA_PHYSICS";
    }
}

const char* MultiPhysicsSimWorkspace::simSpaceMediumName() const {
    switch (m_draftConfig.simSpaceMedium) {

    case SimSpaceMedium::Air:
        return "AIR";

    case SimSpaceMedium::Vacuum:
    default:
        return "VACUUM";
    }
}