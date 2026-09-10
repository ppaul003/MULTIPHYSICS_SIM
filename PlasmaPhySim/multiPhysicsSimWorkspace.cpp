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
        renderConfiguredGrid(services, m_draftConfig.gridLayout);
        if (m_layer2Selection == Layer2Row::VoxelSpawn) {
            renderSelectedSpawnRegion(services);
        }
        return;

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
        return handleLayer2Input(input, services);

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
            return true;
        }

        if (m_layer1Selection == Layer1Row::Configure) {

            if (m_draftConfig.gridLayout == GridLayout::Dynamic) {
                m_statusLine = "DYNAMIC GRID is unavailable for this pass.";
                m_statusTone = WorkspaceStatusTone::Warning;
                return true;
            }
            
            if (m_draftConfig.multphysicsMode != MultiphysicsMode::PlasmaPhy) {
                m_statusLine = "SELECT PLASMA_PHYSICS TO CONFIGURE WORKSPACE.";
                m_statusTone = WorkspaceStatusTone::Warning;
                return true;
            }

            m_layer2Selection = Layer2Row::ParticleSpecies;
            m_textEntry.cancel();
            
            m_statusLine = "READY: PLASMA_PHYSICS workspace configuration.";
            m_statusTone = WorkspaceStatusTone::Ready;

            services.arbiter->setApplicationLayer(
                TheArbiter::ApplicationLayer::WORKSPACE_CONFIGURATION
            );
            
            return true;
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

bool MultiPhysicsSimWorkspace::handleLayer2Input(
    const WorkspaceInputEvent& input, 
    WorkspaceServices& services) {

    if (m_textEntry.isActive())
        return handleLayer2TextEntry(input);

    switch (input.action) {

    case WorkspaceInputAction::Previous:
        moveLayer2Cursor(-1);
        return true;

    case WorkspaceInputAction::Next:
        moveLayer2Cursor(+1);
        return true;

    case WorkspaceInputAction::Decrease:
        adjustLayer2Value(-1);
        return true;

    case WorkspaceInputAction::Increase:
        adjustLayer2Value(+1);
        return true;

    case WorkspaceInputAction::Activate:
        if (m_layer2Selection == Layer2Row::TotalGasDensity) {
            beginGasDensityEntry();
            return true;
        }

        if (m_layer2Selection == Layer2Row::RunSimulation) {
            m_statusLine = "LAYER 3 SIMULATION RUNTIME unavailable in current pass.";
            m_statusTone = WorkspaceStatusTone::Warning;
            return true;
        }

        return true;

    case WorkspaceInputAction::Back:
        m_textEntry.cancel();
        m_statusLine = "READY: MULTPHY_SIM WORKSPACE.";
        m_statusTone = WorkspaceStatusTone::Ready;
        services.arbiter->setApplicationLayer(
            TheArbiter::ApplicationLayer::DOMAIN_SELECTION
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

void MultiPhysicsSimWorkspace::renderSelectedSpawnRegion(WorkspaceServices& services) const {
    if (!services.renderer) return;

    SpawnDensityRegion3D selectedRegion;
    if (!m_spawnDensityGrid.region(
        m_baseVoxelGrid,
        m_draftConfig.spawnVoxelId,
        selectedRegion)) {

        return;
    }

    // Draw each constituent base voxel so the 2x2x2 physical subdivision
    // remains visible, then reinforce the continuous composite boundary.
    for (const SpatialVoxelRegion& baseVoxel :
        selectedRegion.constituentBaseVoxels) {
        services.renderer->drawHighlightedVoxel(
            baseVoxel.center,
            baseVoxel.halfExtent,
            2.0f
        );
    }

    services.renderer->drawHighlightedVoxel(
        selectedRegion.center,
        selectedRegion.halfExtent,
        4.0f
    );
}

void MultiPhysicsSimWorkspace::beginGasDensityEntry() {
    if (m_textEntry.beginUnsignedInteger(
        "TOTAL GAS DESNITY",
        0,
        kParticleCapacity,
        m_draftConfig.totalGasDensity)) {

        m_statusLine = "ENTER TOTAL GAS DENSITY; E/Enter commits.";
        m_statusTone = WorkspaceStatusTone::Neutral;
    }
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

WorkspacePresentation
MultiPhysicsSimWorkspace::buildLayer2Presentation() const {
    
    WorkspacePresentation p;

    p.panelVisible = true;
    p.workspaceName = "LAYER 2 -> MULTIPHYSICS_SIM CONFIGURATION";
    p.layerLabel = "MODE: MULTIPHYSICS_SIM";

    WorkspacePanelSection section;

    section.rows.push_back(makeRow(
        "[1]: PARTICLE SPECIES", 
        particleSpeciesName(), 
        m_layer2Selection == Layer2Row::ParticleSpecies
    ));

    // Density row supports interactive text entry
    if (m_textEntry.isActive() && m_layer2Selection == 
        Layer2Row::TotalGasDensity) {

        section.rows.push_back(makeRow(
            "[2]: TOTAL GAS DENSITY", 
            ":=" + m_textEntry.getBuffer() + "m⁻³", 
            true
        ));
    }
    else {
        section.rows.push_back(makeRow(
            "[2]: TOTAL GAS DENSITY", 
            to_string(m_draftConfig.totalGasDensity) + "m^{-3}", 
            m_layer2Selection == Layer2Row::TotalGasDensity
        ));
    }

    {
        ostringstream value;
        value
            << fixed
            << setprecision(2)
            << m_draftConfig.ionizationFraction;

        section.rows.push_back(makeRow(
            "[3]: IONIZATION", 
            value.str(), 
            m_layer2Selection == Layer2Row::IonizationFraction
        ));
    }

    section.rows.push_back(makeRow(
        "[4]: ELECTRON TEMP", 
        to_string(m_draftConfig.electronTemperature) + "eV", 
        m_layer2Selection == Layer2Row::ElectronTemperature
    ));

    section.rows.push_back(makeRow(
        "[5]: ION TEMP", 
        to_string(m_draftConfig.ionTemperature) + "eV", 
        m_layer2Selection == Layer2Row::IonTemperature
    ));

    section.rows.push_back(makeRow(
        "[6]: NEUTRAL TEMP", 
        to_string(m_draftConfig.neutralTemperature) + " K", 
        m_layer2Selection == Layer2Row::NeutralTemperature
    ));

    section.rows.push_back(makeRow(
        "[7]: SELECT VOXEL SPAWN", 
        voxelText(m_draftConfig.spawnVoxelId), 
        m_layer2Selection == Layer2Row::VoxelSpawn
    ));

    section.rows.push_back(makeRow(
        "[8]: PRESS E TO RUN SIM", 
        "", 
        m_layer2Selection == Layer2Row::RunSimulation
    ));

    p.sections.push_back(section);

    if (m_statusTone != WorkspaceStatusTone::Ready) {
        p.statusLine = m_statusLine;
        p.statusTone = m_statusTone;
    }

    // Plasma population summary
    p.postStatusLines.push_back(
        "NEUTRAL " + string(particleSpeciesName()) + 
        ": " + to_string(neutralCount())
    );

    p.postStatusLines.push_back(
        "IONIZED " + string(particleSpeciesName()) +
        ": " + to_string(ionCount())
    );

    p.postStatusLines.push_back("FREE ELECTRONS: " + to_string(electronCount()));
    p.postStatusLines.push_back("--------------------");
    p.postStatusLines.push_back(
        string(particleSpeciesName()) + " TOTAL: " + 
        to_string(m_draftConfig.totalGasDensity)
    );

    p.postStatusLines.push_back(
        "SIM MARKERS: " + to_string(requestedMarkerCount()) + 
        "/" + to_string(kParticleCapacity)
    );

    p.footerLine1 = "W/S: Select row    A/D: Change value    E: Activate / Enter";
    p.footerLine2 = "Q: Return to Layer 1    ESC: Exit";

    return p;
}

WorkspacePresentation MultiPhysicsSimWorkspace::buildPresentation() const {
    if (!m_arbiter) return {};

    switch (m_arbiter->getApplicationLayer()) {
    case TheArbiter::ApplicationLayer::DOMAIN_SELECTION:
        return buildLayer1Presentation();

    case TheArbiter::ApplicationLayer::WORKSPACE_CONFIGURATION:
        return buildLayer2Presentation();

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
    const int step = direction < 0 ? -1 : +1;
    m_layer1Selection = static_cast<Layer1Row>(
        (current + step + count) % count
    );
}

void MultiPhysicsSimWorkspace::moveLayer2Cursor(int direction) {
    if (direction == 0) return;
    const int count = static_cast<int>(Layer2Row::Count);
    const int current = static_cast<int>(m_layer2Selection);
    const int step = direction < 0 ? -1 : +1;
    m_layer2Selection = static_cast<Layer2Row>(
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

void MultiPhysicsSimWorkspace::adjustLayer2Value(int direction) {
    if (direction == 0) return;

    const int step = direction < 0 ? -1 : +1;

    switch (m_layer2Selection) {

    case Layer2Row::ParticleSpecies: {
        const int count = static_cast<int>(ParticleSpecies::Count);
        const int current = static_cast<int>(m_draftConfig.particleSpecies);
        m_draftConfig.particleSpecies = static_cast<ParticleSpecies>(
            (current + step + count) % count
        );
        break;
    }
    case Layer2Row::IonizationFraction:
        m_draftConfig.ionizationFraction = std::clamp(
            m_draftConfig.ionizationFraction + static_cast<float>(step) * 0.01f, 
            0.0f, 
            1.0f
        );
        break;

    case Layer2Row::ElectronTemperature:
        m_draftConfig.electronTemperature = std::max(
            0.0f, 
            m_draftConfig.electronTemperature + step * 0.1f
        );
        break;

    case Layer2Row::IonTemperature:
        m_draftConfig.ionTemperature = std::max(
            0.0f,
            m_draftConfig.ionTemperature + step * 0.1f
        );
        break;

    case Layer2Row::NeutralTemperature:
        m_draftConfig.neutralTemperature = std::max(
            0.0f,
            m_draftConfig.neutralTemperature + step * 0.1f
        );
        break;

    case Layer2Row::VoxelSpawn: {
        const unsigned int count = m_spawnDensityGrid.regionCount(
            m_baseVoxelGrid
        );

        if (count == 0) break;
        const int current = static_cast<int>(m_draftConfig.spawnVoxelId);

        m_draftConfig.spawnVoxelId = static_cast<unsigned int>(
            (current + step + static_cast<int>(count)) % static_cast<int>(count)
        );
        break;
    }

    case Layer2Row::RunSimulation:
    case Layer2Row::Count:
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

bool MultiPhysicsSimWorkspace::handleLayer2TextEntry(const WorkspaceInputEvent& input) {

    if (input.action == WorkspaceInputAction::Back ||
        input.rawKey == 'q' || input.rawKey == 'Q') {

        m_textEntry.cancel();
        m_statusLine = "TOTAL GAS DENSITY entry cancelled.";
        m_statusTone = WorkspaceStatusTone::Neutral;

        return true;
    }

    const TextEntryAction action = m_textEntry.handleRawKey(input.rawKey);

    switch (action) {

    case TextEntryAction::Committed: {
        unsigned int value = 0;

        if (m_textEntry.tryGetCommittedUnsigned(value)) {
            m_draftConfig.totalGasDensity = value;
            m_statusLine = "READY: TOTAL GAS DENSITY committed.";
            m_statusTone = WorkspaceStatusTone::Ready;
        }
        else {
            m_statusLine = "TOTAL GAS DENSITY was not committed.";
            m_statusTone = WorkspaceStatusTone::Warning;
        }

        break;
    }
    case TextEntryAction::Cancelled:
        m_statusLine = "TOTAL GAS DENSITY entry cancelled.";
        m_statusTone = WorkspaceStatusTone::Neutral;
        break;


    case TextEntryAction::Rejected:
        m_statusLine = m_textEntry.getStatusMessage();
        m_statusTone = WorkspaceStatusTone::Warning;
        break;

    case TextEntryAction::Changed:
        m_statusLine = "ENTER TOTAL GAS DENSITY; E/ENTER commits.";
        m_statusTone = WorkspaceStatusTone::Neutral;
        break;

    case TextEntryAction::None:
    default:
        break;
    }

    return true;
}

unsigned int MultiPhysicsSimWorkspace::ionCount() const {
    const float value = static_cast<float>(m_draftConfig.totalGasDensity) *
        m_draftConfig.ionizationFraction;

    return static_cast<unsigned int>(round(value));
}

unsigned int MultiPhysicsSimWorkspace::neutralCount() const {
    return m_draftConfig.totalGasDensity - ionCount();
}

unsigned int MultiPhysicsSimWorkspace::electronCount() const {
    return ionCount();
}

unsigned int MultiPhysicsSimWorkspace::requestedMarkerCount() const {
    return neutralCount() + ionCount() + electronCount();
}

string MultiPhysicsSimWorkspace::voxelText(unsigned int voxelId) {
    ostringstream stream;
    stream << "VOXEL_" << setw(3) << setfill('0') << voxelId;
    return stream.str();
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

const char* MultiPhysicsSimWorkspace::particleSpeciesName() const {
    switch (m_draftConfig.particleSpecies) {

    case ParticleSpecies::Hydrogen:
        return "HYDROGEN";

    case ParticleSpecies::Helium:
        return "HELIUM";

    case ParticleSpecies::Argon:
    default:
        return "ARGON";
    }
}