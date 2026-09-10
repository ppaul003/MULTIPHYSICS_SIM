#ifndef NDMSM_MULTIPHYSICS_SIM_WORKSPACE_H
#define NDMSM_MULTIPHYSICS_SIM_WORKSPACE_H

#include <memory>
#include <string>
#include <vector>

#include <cuda_runtime.h>
#include <vector_functions.h>
#include <vector_types.h>

#include "IWorkspaceEM.h"
#include "TextEntry.h"
#include "TheArbiterEM.h"
#include "particleSystem.h"

class MultiPhysicsSimWorkspace final : public IWorkspace {
public:

    bool initialize(WorkspaceServices& services) override;
    void enter(WorkspaceServices& services) override;
    void exit(WorkspaceServices& services) override;

    void update(
        const WorkspaceFrameContext& frame,
        WorkspaceServices& services
    ) override;

    void render(
        const WorkspaceFrameContext& frame,
        WorkspaceServices& services
    ) override;

    bool handleInput(
        const WorkspaceInputEvent& input,
        WorkspaceServices& services
    ) override;

    WorkspacePresentation buildPresentation() const override;

private:
    enum class Layer1Row {
        WorkspaceSelection = 0,
        GridLayout,
        MultiphysicsMode,
        SimSpaceMedium,
        Configure,
        Count
    };

    enum class Layer2Row {
        ParticleSpecies = 0,
        TotalGasDensity,
        IonizationFraction,
        ElectronTemperature,
        IonTemperature,
        NeutralTemperature,
        VoxelSpawn,
        RunSimulation,
        Count
    };

    enum class GridLayout {
        None = 0,
        MajorGrid,
        Dynamic,
        Count
    };

    enum class MultiphysicsMode {
        Electrodynamics = 0,
        EmWave,
        PlasmaPhy,
        Count
    };

    enum class SimSpaceMedium {
        Vacuum = 0,
        Air,
        Count
    };
    
    enum class ParticleSpecies {
        Hydrogen = 0,
        Helium, 
        Argon,
        Count
    };

    enum class Layer3CameraView {
        Orbit = 0,
        Free
    };

    struct DraftConfig {

        // --- Layer 1 --- 
        GridLayout gridLayout = GridLayout::None;
        MultiphysicsMode multphysicsMode = MultiphysicsMode::PlasmaPhy;
        SimSpaceMedium simSpaceMedium = SimSpaceMedium::Vacuum;

        // --- Layer 2 ---
        ParticleSpecies particleSpecies = ParticleSpecies::Argon;
        unsigned int totalGasDensity = 0;
        float ionizationFraction = 0.10f;
        float electronTemperature = 0.0f;
        float ionTemperature = 0.0f;
        float neutralTemperature = 0.0f;
        unsigned int spawnVoxelId = 0;
    };

    struct ParticleVisual {
        float renderRadius;
        float4 color;
        float emissiveStrength;
    };

    struct ParticlePhysics {
        double mass;
        double charge;
    };

    WorkspacePresentation buildLayer1Presentation() const;
    WorkspacePresentation buildLayer2Presentation() const;

    bool handleLayer1Input(const WorkspaceInputEvent& input, WorkspaceServices& services);
    bool handleLayer2Input(const WorkspaceInputEvent& input, WorkspaceServices& services);

    bool handleLayer2TextEntry(const WorkspaceInputEvent& input);

    unsigned int neutralCount() const;
    unsigned int ionCount() const;
    unsigned int electronCount() const;
    unsigned int requestedMarkerCount() const;

    void renderConfiguredGrid(WorkspaceServices& services, GridLayout layout) const;
    void renderSelectedSpawnRegion(WorkspaceServices& services) const;

    void beginGasDensityEntry();

    void moveLayer1Cursor(int direction);
    void moveLayer2Cursor(int direction);

    void adjustLayer1Value(int direction, WorkspaceServices& services);
    void adjustLayer2Value(int direction);

    void refreshLayer1Status();

    bool isVoxelSpawnRowSelected() const;
    int voxelSpawnRowIndex() const;

    static std::string voxelText(unsigned int voxelId);

    const char* gridLayoutName() const;
    const char* multiphysicsModeName() const;
    const char* simSpaceMediumName() const;
    const char* particleSpeciesName() const;


private:
    static constexpr float kSimBoxSizeM = 4.0f;
    static constexpr float kSimHalfBoxM = kSimBoxSizeM * 0.5f;
    static constexpr float kMaximumSupportedRadius = 0.0156f;
    static constexpr unsigned int kParticleCapacity = 16384;
    static constexpr unsigned int kMajorGridEvery = 8;
    static constexpr unsigned int kGridSize = 64;
    static constexpr unsigned int kDefaultCountStep = 100;
    static constexpr unsigned int kResetSeed = 1973;
    static constexpr float kCellSizeM =
        kSimBoxSizeM / static_cast<float>(kGridSize);

    std::unique_ptr<ParticleSystem> m_particleSystem;
    std::string m_statusLine = "READY: MULTIPHY_SIM MODE ONLINE.";
    std::vector<float> m_radii;
    
    TheArbiter* m_arbiter = nullptr;
    DraftConfig m_draftConfig;
    SpatialVoxelGrid3D m_baseVoxelGrid;
    SpawnDensityRegionGrid3D m_spawnDensityGrid;
    TextEntrySession m_textEntry;

    Layer1Row m_layer1Selection = Layer1Row::WorkspaceSelection;
    Layer2Row m_layer2Selection = Layer2Row::ParticleSpecies;

    WorkspaceStatusTone m_statusTone = WorkspaceStatusTone::Ready;

    bool m_subLayerPanelOpen = false;
    bool m_displaySliders = false;
    bool m_initialized = false;
    bool m_active = false;
    bool m_paused = true;
    bool m_runtimeEnabled = false;

    float m_elapsedSimulationTime = 0.0f;
};

#endif
