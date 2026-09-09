#include "ParticleSimWorkspace.h"

#include "rendererEM_Euclid.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace glm;

namespace {

constexpr float kRadiusPresets[] = {
	0.0039f,
	0.0046f,
	0.0054f,
	0.0061f,
	0.0068f,
	0.0076f,
	0.0083f,
	0.0091f,
	0.0098f,
	0.0105f,
	0.0113f,
	0.0120f,
	0.0127f,
	0.0135f,
	0.0142f,
	0.0149f,
	0.0156f
};

constexpr int kRadiusPresetCount =
	static_cast<int>(sizeof(kRadiusPresets) / sizeof(kRadiusPresets[0]));

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

WorkspacePanelRow makeSummaryRow(const string& label) {
	WorkspacePanelRow row;
	row.label = label;
	return row;
}

} // namespace

bool ParticleSimWorkspace::initialize(WorkspaceServices& services) {
	if (m_initialized) return true;
	if (!services.renderer || !services.arbiter) return false;

	m_arbiter = services.arbiter;
	m_baseVoxelGrid.dimensions = ivec3(8, 8, 8);
	m_baseVoxelGrid.origin = vec3(-2.0f, -2.0f, -2.0f);
	m_baseVoxelGrid.voxelEdgeM = 0.5f;
	m_radii.assign(kParticleCapacity, 0.0f);

	m_particleSystem = make_unique<ParticleSystem>(
		kParticleCapacity,
		m_gridDimensions,
		true
	);
	m_particleSystem->setSimulationDomain(kSimulationBoxSizeM);
	if (!m_particleSystem->setActiveParticleCount(0)) return false;

	m_initialized = true;
	return true;
}

void ParticleSimWorkspace::enter(WorkspaceServices& services) {
	if (!m_arbiter) m_arbiter = services.arbiter;
	if (!m_initialized) return;

	m_active = true;
	m_paused = true;
	m_runtimeEnabled = false;
	m_elapsedSimulationTime = 0.0f;
	m_textEntry.cancel();
}

void ParticleSimWorkspace::exit(WorkspaceServices& services) {
	(void)services;
	m_active = false;
	m_paused = true;
	m_runtimeEnabled = false;
	m_textEntry.cancel();
}

void ParticleSimWorkspace::update(
	const WorkspaceFrameContext& frame,
	WorkspaceServices& services) {

	if (!m_active ||
		!m_particleSystem ||
		!services.arbiter ||
		services.arbiter->getApplicationLayer() !=
			TheArbiter::ApplicationLayer::ACTIVE_WORKSPACE ||
		!m_runtimeEnabled ||
		m_paused) {
		return;
	}

	m_particleSystem->update(frame.deltaTime);
	m_elapsedSimulationTime += frame.deltaTime;
}

void ParticleSimWorkspace::render(
	const WorkspaceFrameContext& frame,
	WorkspaceServices& services) {

	if (!frame.displayEnabled || !services.renderer || !services.arbiter) {
		return;
	}

	switch (services.arbiter->getApplicationLayer()) {
	case TheArbiter::ApplicationLayer::DOMAIN_SELECTION:
		renderConfiguredGrid(services, m_draftConfig.gridLayout);
		return;

	case TheArbiter::ApplicationLayer::WORKSPACE_CONFIGURATION:
		renderConfiguredGrid(services, m_draftConfig.gridLayout);
		if (isVoxelSpawnRowSelected()) {
			renderSelectedSpawnRegion(services);
		}
		return;

	case TheArbiter::ApplicationLayer::ACTIVE_WORKSPACE:
		renderConfiguredGrid(services, m_runtimeConfig.gridLayout);
		renderActiveParticles(services);
		return;

	case TheArbiter::ApplicationLayer::GLOBAL_SHELL:
	default:
		return;
	}
}

bool ParticleSimWorkspace::handleInput(
	const WorkspaceInputEvent& input,
	WorkspaceServices& services) {

	if (!m_active || !services.arbiter) return false;
	if (!m_arbiter) m_arbiter = services.arbiter;

	switch (services.arbiter->getApplicationLayer()) {
	case TheArbiter::ApplicationLayer::DOMAIN_SELECTION:
		return handleLayer1Input(input, services);

	case TheArbiter::ApplicationLayer::WORKSPACE_CONFIGURATION:
		return handleLayer2Input(input, services);

	case TheArbiter::ApplicationLayer::ACTIVE_WORKSPACE:
		return handleLayer3Input(input, services);

	case TheArbiter::ApplicationLayer::GLOBAL_SHELL:
	default:
		return false;
	}
}

bool ParticleSimWorkspace::handleLayer1Input(
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
				m_layer2Selection = 0;
				m_paused = true;
				m_runtimeEnabled = false;
				m_textEntry.cancel();
				m_statusLine = "READY: PARTICLE_SIM workspace configuration.";
				m_statusTone = WorkspaceStatusTone::Ready;
				services.arbiter->setApplicationLayer(
					TheArbiter::ApplicationLayer::WORKSPACE_CONFIGURATION
				);
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

bool ParticleSimWorkspace::handleLayer2Input(
	const WorkspaceInputEvent& input,
	WorkspaceServices& services) {

	if (m_textEntry.isActive()) {
		return handleTextEntry(input);
	}

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
		if (m_layer2Selection == 0 &&
			m_draftConfig.colorMode == ColorMode::RGB) {
			beginParticleAmountEntry();
		}
		else if (m_layer2Selection == runSimulationRowIndex()) {
			m_paused = true;
			m_runtimeEnabled = false;

			if (applyRuntimeConfig()) {
				m_elapsedSimulationTime = 0.0f;
				m_paused = false;
				m_runtimeEnabled = true;
				m_statusLine = "STATUS: RUNNING";
				m_statusTone = WorkspaceStatusTone::Ready;
				services.arbiter->setApplicationLayer(
					TheArbiter::ApplicationLayer::ACTIVE_WORKSPACE
				);
			}
			else {
				m_statusLine = "PARTICLE CONFIGURATION IS INVALID.";
				m_statusTone = WorkspaceStatusTone::Warning;
			}
		}
		return true;

	case WorkspaceInputAction::Back:
		m_paused = true;
		m_runtimeEnabled = false;
		m_textEntry.cancel();
		m_statusLine = "READY: CUDA PARTICLE BASELINE.";
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

bool ParticleSimWorkspace::handleLayer3Input(
	const WorkspaceInputEvent& input,
	WorkspaceServices& services) {

	if (input.action != WorkspaceInputAction::Back) return false;

	m_paused = true;
	m_runtimeEnabled = false;
	m_statusLine = "STATUS: PAUSED";
	m_statusTone = WorkspaceStatusTone::Neutral;
	services.arbiter->setApplicationLayer(
		TheArbiter::ApplicationLayer::WORKSPACE_CONFIGURATION
	);
	return true;
}

bool ParticleSimWorkspace::handleTextEntry(
	const WorkspaceInputEvent& input) {

	if (input.action == WorkspaceInputAction::Back ||
		input.rawKey == 'q' || input.rawKey == 'Q') {
		m_textEntry.cancel();
		m_statusLine = "RGB particle amount entry cancelled.";
		m_statusTone = WorkspaceStatusTone::Neutral;
		return true;
	}

	const TextEntryAction action = m_textEntry.handleRawKey(input.rawKey);
	switch (action) {
	case TextEntryAction::Committed: {
		unsigned int committedValue = 0;
		if (m_textEntry.tryGetCommittedUnsigned(committedValue)) {
			setSelectedColorCount(committedValue);
			m_statusLine = "READY: RGB particle amount committed.";
			m_statusTone = WorkspaceStatusTone::Ready;
		}
		else {
			m_statusLine = "RGB particle amount was not committed.";
			m_statusTone = WorkspaceStatusTone::Warning;
		}
		break;
	}

	case TextEntryAction::Cancelled:
		m_statusLine = "RGB particle amount entry cancelled.";
		m_statusTone = WorkspaceStatusTone::Neutral;
		break;

	case TextEntryAction::Rejected:
		m_statusLine = m_textEntry.getStatusMessage();
		m_statusTone = WorkspaceStatusTone::Warning;
		break;

	case TextEntryAction::Changed:
		m_statusLine = "ENTER RGB particle amount; E/ENTER commits.";
		m_statusTone = WorkspaceStatusTone::Neutral;
		break;

	case TextEntryAction::None:
	default:
		break;
	}

	return true;
}

void ParticleSimWorkspace::renderConfiguredGrid(
	WorkspaceServices& services,
	GridLayout layout) const {

	if (!services.renderer) return;

	EuclidRenderer::UniformGrid grid;
	grid.dimensions = ivec3(
		static_cast<int>(kGridSize),
		static_cast<int>(kGridSize),
		static_cast<int>(kGridSize)
	);
	grid.origin = m_baseVoxelGrid.origin;
	grid.cellSize = vec3(
		kSimulationBoxSizeM / static_cast<float>(kGridSize)
	);
	grid.majorEvery = static_cast<int>(kMajorGridEvery);

	EuclidRenderer::GridDisplay display;
	display.boundary = true;
	display.majorGrid = layout == GridLayout::MajorGrid;
	display.minorGrid = false;
	display.axes = false;

	services.renderer->drawUniformGrid(grid, display);
}

void ParticleSimWorkspace::renderSelectedSpawnRegion(
	WorkspaceServices& services) const {

	if (!services.renderer) return;

	SpawnDensityRegion3D selectedRegion;
	if (!m_spawnDensityGrid.region(
		m_baseVoxelGrid,
		m_draftConfig.spawnVoxelId,
		selectedRegion
	)) {
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

void ParticleSimWorkspace::renderActiveParticles(
	WorkspaceServices& services) {

	if (!services.renderer || !m_particleSystem || m_activeCount == 0) return;

	services.renderer->setParticleSystem(m_particleSystem.get());
	services.renderer->setVertexBuffer(
		m_particleSystem->getCurrentReadBuffer(),
		static_cast<int>(m_activeCount)
	);
	services.renderer->setColorBuffer(m_particleSystem->getColorBuffer());
	services.renderer->setRadius(
		m_radii.data(),
		static_cast<int>(m_activeCount)
	);
	services.renderer->setParticleRadius(m_runtimeConfig.placementRadius);
	services.renderer->display(EuclidRenderer::PARTICLE_SPHERES);
}

WorkspacePresentation ParticleSimWorkspace::buildLayer1Presentation() const {
	WorkspacePresentation p;
	p.panelVisible = true;
	p.workspaceName = "LAYER 1 -> MULPHY_SIM WORKSPACE CONFIGURATION";
	p.layerLabel = "MODE: PARTICLE_SIM";

	WorkspacePanelSection section;
	section.rows.push_back(makeRow(
		"[1]: MULPHY_SIM SELECTION",
		"PARTICLE_SIM",
		m_layer1Selection == Layer1Row::WorkspaceSelection
	));
	section.rows.push_back(makeRow(
		"[2]: GRID LAYOUT",
		gridLayoutName(),
		m_layer1Selection == Layer1Row::GridLayout
	));
	section.rows.push_back(makeRow(
		"[3]: COLOR MODE",
		colorModeName(),
		m_layer1Selection == Layer1Row::ColorMode
	));
	section.rows.push_back(makeRow(
		"[4]: PARTICLE RADIUS",
		radiusModeName(),
		m_layer1Selection == Layer1Row::RadiusMode
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

WorkspacePresentation ParticleSimWorkspace::buildLayer2Presentation() const {
	WorkspacePresentation p;
	p.panelVisible = true;
	p.workspaceName = "LAYER 2 -> PARTICLE_SIM CONFIGURATION";
	p.layerLabel = "MODE: PARTICLE_SIM";

	WorkspacePanelSection section;
	if (m_draftConfig.colorMode == ColorMode::Default) {
		section.rows.push_back(makeRow(
			"[1]: PARTICLE AMOUNT",
			to_string(m_draftConfig.defaultParticleCount) + "/" +
				to_string(m_capacity),
			m_layer2Selection == 0
		));
	}
	else {
		ostringstream amount;
		amount << "[1]: PARTICLE AMOUNT {" << colorChannelName() << "}[";
		if (m_textEntry.isActive()) amount << ":=" << m_textEntry.getBuffer();
		else amount << selectedColorCount();

		amount << "]";
		section.rows.push_back(
			makeRow(amount.str(), "", m_layer2Selection == 0)
		);
	}

	section.rows.push_back(
		makeRow("[2]: PARTICLE RESET MODE", resetModeName(), m_layer2Selection == 1)
	);

	if (m_draftConfig.radiusMode == RadiusMode::Uniform) {
		section.rows.push_back(
			makeRow("[3]: PARTICLE RADIUS", radiusText(m_draftConfig.uniformRadius), m_layer2Selection == 2)
		);

		section.rows.push_back(
			makeRow("[4]: SELECT VOXEL SPAWN", voxelText(m_draftConfig.spawnVoxelId), m_layer2Selection == 3)
		);

		section.rows.push_back(
			makeRow("[5]: PRESS E TO RUN SIM", "", m_layer2Selection == 4)
		);
	}
	else {
		section.rows.push_back(
			makeRow("[3]: MIN RADIUS", radiusText(m_draftConfig.minimumRadius), m_layer2Selection == 2)
		);

		section.rows.push_back(
			makeRow("[4]: MAX RADIUS",radiusText(m_draftConfig.maximumRadius), m_layer2Selection == 3)
		);

		section.rows.push_back(
			makeRow("[5]: SELECT VOXEL SPAWN", voxelText(m_draftConfig.spawnVoxelId), m_layer2Selection == 4)
		);

		section.rows.push_back(
			makeRow("[6]: PRESS E TO RUN SIM", "", m_layer2Selection == 5)
		);
	}

	p.sections.push_back(section);
	p.statusLine = m_statusLine;
	p.statusTone = m_statusTone;
	if (m_draftConfig.colorMode == ColorMode::RGB) {

		ostringstream rgbSummary;

		rgbSummary
			<< "RED " << m_draftConfig.redCount
			<< " | GREEN " << m_draftConfig.greenCount
			<< " | BLUE " << m_draftConfig.blueCount;

		p.postStatusLines.push_back(
			rgbSummary.str()
		);

		p.postStatusLines.push_back(
			"TOTAL " +
			to_string(requestedParticleCount()) +
			"/" +
			to_string(m_capacity)
		);
	}

	p.footerLine1 = "W/S: Select row    A/D: Change value    E: Activate / Enter";
	p.footerLine2 = "Q: Return to Layer 1    ESC: Exit";
	return p;
}

WorkspacePresentation ParticleSimWorkspace::buildLayer3Presentation() const {
	WorkspacePresentation p;
	p.panelVisible = true;
	p.workspaceName = "LAYER 3 -> PARTICLE_SIM ACTIVE WORKSPACE";
	p.layerLabel = "MODE: PARTICLE_SIM";

	WorkspacePanelSection section;
	section.rows.push_back(makeSummaryRow(
		"ACTIVE VOXEL: " + voxelText(m_runtimeConfig.selectedSpawnRegionId)
	));
	section.rows.push_back(makeSummaryRow(
		"PARTICLES: " + to_string(m_runtimeConfig.activeMacroParticleCount) +
		"/" + to_string(m_runtimeConfig.capacity)
	));
	section.rows.push_back(makeSummaryRow(
		"VOXEL VOLUME: " + to_string(m_runtimeConfig.selectedSpawnVolumeM3) +
		" m^3"
	));
	p.sections.push_back(section);

	p.statusLine = m_runtimeEnabled && !m_paused
		? "STATUS: RUNNING"
		: "STATUS: PAUSED";
	p.statusTone = m_runtimeEnabled && !m_paused
		? WorkspaceStatusTone::Ready
		: WorkspaceStatusTone::Neutral;
	p.footerLine1 = "Q: Return to PARTICLE_SIM configuration";
	p.footerLine2 = "ESC: Exit";
	return p;
}

WorkspacePresentation ParticleSimWorkspace::buildPresentation() const {
	if (!m_arbiter) return {};

	switch (m_arbiter->getApplicationLayer()) {
	case TheArbiter::ApplicationLayer::DOMAIN_SELECTION:
		return buildLayer1Presentation();

	case TheArbiter::ApplicationLayer::WORKSPACE_CONFIGURATION:
		return buildLayer2Presentation();

	case TheArbiter::ApplicationLayer::ACTIVE_WORKSPACE:
		return buildLayer3Presentation();

	case TheArbiter::ApplicationLayer::GLOBAL_SHELL:
	default:
		return {};
	}
}

WorkspacePresentation
ParticleSimWorkspace::buildLayer1TransitionPresentation() const {
	WorkspacePresentation p = buildLayer1Presentation();
	p.statusLine = "AUTO: Entering MULPHY_SIM Domain...";
	p.statusTone = WorkspaceStatusTone::Transition;
	return p;
}

bool ParticleSimWorkspace::applyRuntimeConfig() {
	if (!m_particleSystem) return false;

	RuntimeConfig resolved;
	if (!resolveRuntimeConfig(resolved)) return false;
	if (!m_particleSystem->setActiveParticleCount(resolved.activeCount)) {
		return false;
	}

	SpawnDensityRegion3D selectedRegion;
	if (!m_spawnDensityGrid.region(
		m_baseVoxelGrid,
		resolved.selectedSpawnRegionId,
		selectedRegion
	)) {
		return false;
	}

	const ParticleSystem::ParticleConfig particleReset =
		resolved.resetMode == ResetMode::Random
		? ParticleSystem::CNFG_RANDOM_RESTART
		: ParticleSystem::CNFG_DEFAULT_RESTART;

	if (!m_particleSystem->resetInBounds(
		particleReset,
		make_float3(
			selectedRegion.minimum.x,
			selectedRegion.minimum.y,
			selectedRegion.minimum.z
		),
		make_float3(
			selectedRegion.maximum.x,
			selectedRegion.maximum.y,
			selectedRegion.maximum.z
		),
		resolved.placementRadius,
		kResetSeed
	)) {
		return false;
	}

	const bool radiusApplied =
		resolved.radiusMode == RadiusMode::Uniform
		? m_particleSystem->setUniformActiveRadii(resolved.uniformRadius)
		: m_particleSystem->setRandomActiveRadii(
			resolved.minimumRadius,
			resolved.maximumRadius,
			kResetSeed
		);
	if (!radiusApplied) return false;

	if (resolved.colorMode == ColorMode::RGB) {
		if (!m_particleSystem->setRGBParticleCounts(
			resolved.redCount,
			resolved.greenCount,
			resolved.blueCount
		)) {
			return false;
		}
	}
	else {
		m_particleSystem->setDefaultColorRamp();
	}

	resolved.selectedSpawnVolumeM3 = selectedRegion.volumeM3;
	resolved.activeMacroParticleCount = resolved.activeCount;
	m_runtimeConfig = resolved;
	m_activeCount = resolved.activeCount;
	m_radii.resize(resolved.activeCount);
	if (resolved.activeCount > 0) {
		m_particleSystem->dumpRadii(
			m_radii.data(),
			resolved.activeCount
		);
	}

	return true;
}

bool ParticleSimWorkspace::resolveRuntimeConfig(
	RuntimeConfig& resolved) const {

	if (m_draftConfig.gridLayout == GridLayout::Dynamic ||
		m_draftConfig.spawnVoxelId >=
			m_spawnDensityGrid.regionCount(m_baseVoxelGrid)) {
		return false;
	}

	const unsigned long long requestedCount =
		m_draftConfig.colorMode == ColorMode::Default
		? static_cast<unsigned long long>(m_draftConfig.defaultParticleCount)
		: static_cast<unsigned long long>(m_draftConfig.redCount) +
			static_cast<unsigned long long>(m_draftConfig.greenCount) +
			static_cast<unsigned long long>(m_draftConfig.blueCount);

	if (requestedCount > m_capacity) return false;

	const bool uniformRadiusValid =
		std::isfinite(m_draftConfig.uniformRadius) &&
		m_draftConfig.uniformRadius > 0.0f &&
		m_draftConfig.uniformRadius <= kMaximumSupportedRadius;

	const bool randomRadiusValid =
		std::isfinite(m_draftConfig.minimumRadius) &&
		std::isfinite(m_draftConfig.maximumRadius) &&
		m_draftConfig.minimumRadius > 0.0f &&
		m_draftConfig.minimumRadius <= m_draftConfig.maximumRadius &&
		m_draftConfig.maximumRadius <= kMaximumSupportedRadius;

	if ((m_draftConfig.radiusMode == RadiusMode::Uniform &&
		!uniformRadiusValid) ||
		(m_draftConfig.radiusMode == RadiusMode::Random &&
		!randomRadiusValid)) {
		return false;
	}

	resolved.capacity = m_capacity;
	resolved.activeCount = static_cast<unsigned int>(requestedCount);
	resolved.redCount = m_draftConfig.redCount;
	resolved.greenCount = m_draftConfig.greenCount;
	resolved.blueCount = m_draftConfig.blueCount;
	resolved.uniformRadius = m_draftConfig.uniformRadius;
	resolved.minimumRadius = m_draftConfig.minimumRadius;
	resolved.maximumRadius = m_draftConfig.maximumRadius;
	resolved.placementRadius =
		m_draftConfig.radiusMode == RadiusMode::Random
		? m_draftConfig.maximumRadius
		: m_draftConfig.uniformRadius;
	resolved.colorMode = m_draftConfig.colorMode;
	resolved.radiusMode = m_draftConfig.radiusMode;
	resolved.resetMode = m_draftConfig.resetMode;
	resolved.gridLayout = m_draftConfig.gridLayout;
	resolved.selectedSpawnRegionId = m_draftConfig.spawnVoxelId;
	return true;
}

void ParticleSimWorkspace::moveLayer1Cursor(int direction) {
	if (direction == 0) return;
	const int count = static_cast<int>(Layer1Row::Count);
	const int current = static_cast<int>(m_layer1Selection);
	const int step = direction < 0 ? -1 : 1;
	m_layer1Selection = static_cast<Layer1Row>(
		(current + step + count) % count
	);
}

void ParticleSimWorkspace::moveLayer2Cursor(int direction) {
	if (direction == 0) return;
	const int count = layer2RowCount();
	const int step = direction < 0 ? -1 : 1;
	m_layer2Selection = (m_layer2Selection + step + count) % count;
}

void ParticleSimWorkspace::adjustLayer1Value(
	int direction,
	WorkspaceServices& services) {

	if (direction == 0) return;

	switch (m_layer1Selection) {
	case Layer1Row::WorkspaceSelection:
		if (services.arbiter) {
			services.arbiter->setActiveWorkspace(
				TheArbiter::WorkspaceId::MULTIPHYSICS_SIM
			);
		}
		return;

	case Layer1Row::GridLayout: {
		const int count = static_cast<int>(GridLayout::Count);
		const int current = static_cast<int>(m_draftConfig.gridLayout);
		const int step = direction < 0 ? -1 : 1;
		m_draftConfig.gridLayout = static_cast<GridLayout>(
			(current + step + count) % count
		);
		break;
	}

	case Layer1Row::ColorMode:
		m_draftConfig.colorMode =
			m_draftConfig.colorMode == ColorMode::Default
			? ColorMode::RGB
			: ColorMode::Default;
		break;

	case Layer1Row::RadiusMode:
		m_draftConfig.radiusMode =
			m_draftConfig.radiusMode == RadiusMode::Uniform
			? RadiusMode::Random
			: RadiusMode::Uniform;
		m_layer2Selection = std::min(
			m_layer2Selection,
			layer2RowCount() - 1
		);
		break;

	case Layer1Row::Configure:
	case Layer1Row::Count:
	default:
		return;
	}

	if (m_draftConfig.gridLayout == GridLayout::Dynamic) {
		m_statusLine = "DYNAMIC GRID is unavailable for this pass.";
		m_statusTone = WorkspaceStatusTone::Warning;
	}
	else {
		m_statusLine = "READY: CUDA PARTICLE BASELINE.";
		m_statusTone = WorkspaceStatusTone::Ready;
	}
}

void ParticleSimWorkspace::adjustLayer2Value(int direction) {
	if (direction == 0) return;
	const int step = direction < 0 ? -1 : 1;

	if (m_layer2Selection == 0) {
		if (m_draftConfig.colorMode == ColorMode::Default) {
			if (step < 0) {
				m_draftConfig.defaultParticleCount =
					m_draftConfig.defaultParticleCount > kDefaultCountStep
					? m_draftConfig.defaultParticleCount - kDefaultCountStep
					: 0;
			}
			else {
				m_draftConfig.defaultParticleCount = std::min(
					m_capacity,
					m_draftConfig.defaultParticleCount + kDefaultCountStep
				);
			}
		}
		else {
			const int count = static_cast<int>(ColorChannel::Count);
			const int current =
				static_cast<int>(m_draftConfig.selectedColorChannel);
			m_draftConfig.selectedColorChannel = static_cast<ColorChannel>(
				(current + step + count) % count
			);
		}
	}
	else if (m_layer2Selection == 1) {
		m_draftConfig.resetMode =
			m_draftConfig.resetMode == ResetMode::Default
			? ResetMode::Random
			: ResetMode::Default;
	}
	else if (m_draftConfig.radiusMode == RadiusMode::Uniform &&
		m_layer2Selection == 2) {
		const int index = std::clamp(
			radiusPresetIndex(m_draftConfig.uniformRadius) + step,
			0,
			kRadiusPresetCount - 1
		);
		m_draftConfig.uniformRadius = radiusPreset(index);
	}
	else if (m_draftConfig.radiusMode == RadiusMode::Random &&
		m_layer2Selection == 2) {
		const int current = radiusPresetIndex(m_draftConfig.minimumRadius);
		const int maximum = radiusPresetIndex(m_draftConfig.maximumRadius);
		m_draftConfig.minimumRadius = radiusPreset(
			std::clamp(current + step, 0, maximum)
		);
	}
	else if (m_draftConfig.radiusMode == RadiusMode::Random &&
		m_layer2Selection == 3) {
		const int current = radiusPresetIndex(m_draftConfig.maximumRadius);
		const int minimum = radiusPresetIndex(m_draftConfig.minimumRadius);
		m_draftConfig.maximumRadius = radiusPreset(
			std::clamp(current + step, minimum, kRadiusPresetCount - 1)
		);
	}
	else if (m_layer2Selection == voxelSpawnRowIndex()) {
		const int count = static_cast<int>(
			m_spawnDensityGrid.regionCount(m_baseVoxelGrid)
		);
		const int current = static_cast<int>(m_draftConfig.spawnVoxelId);
		m_draftConfig.spawnVoxelId = static_cast<unsigned int>(
			(current + step + count) % count
		);
	}

	m_statusLine = "READY: PARTICLE_SIM workspace configuration.";
	m_statusTone = WorkspaceStatusTone::Ready;
}

void ParticleSimWorkspace::beginParticleAmountEntry() {
	const unsigned int maximum = m_capacity - otherColorCount();
	const unsigned int initial = std::min(selectedColorCount(), maximum);
	if (m_textEntry.beginUnsignedInteger(
		"PARTICLE AMOUNT",
		0,
		maximum,
		initial
	)) {
		m_statusLine = "ENTER RGB particle amount; E/ENTER commits.";
		m_statusTone = WorkspaceStatusTone::Neutral;
	}
}

int ParticleSimWorkspace::layer2RowCount() const {
	return m_draftConfig.radiusMode == RadiusMode::Random ? 6 : 5;
}

int ParticleSimWorkspace::voxelSpawnRowIndex() const {
	return m_draftConfig.radiusMode == RadiusMode::Random ? 4 : 3;
}

int ParticleSimWorkspace::runSimulationRowIndex() const {
	return m_draftConfig.radiusMode == RadiusMode::Random ? 5 : 4;
}

bool ParticleSimWorkspace::isVoxelSpawnRowSelected() const {
	return m_layer2Selection == voxelSpawnRowIndex();
}

unsigned int ParticleSimWorkspace::selectedColorCount() const {
	switch (m_draftConfig.selectedColorChannel) {
	case ColorChannel::Green: return m_draftConfig.greenCount;
	case ColorChannel::Blue: return m_draftConfig.blueCount;
	case ColorChannel::Red:
	default:
		return m_draftConfig.redCount;
	}
}

unsigned int ParticleSimWorkspace::otherColorCount() const {
	switch (m_draftConfig.selectedColorChannel) {
	case ColorChannel::Green:
		return m_draftConfig.redCount + m_draftConfig.blueCount;
	case ColorChannel::Blue:
		return m_draftConfig.redCount + m_draftConfig.greenCount;
	case ColorChannel::Red:
	default:
		return m_draftConfig.greenCount + m_draftConfig.blueCount;
	}
}

void ParticleSimWorkspace::setSelectedColorCount(unsigned int value) {
	const unsigned int boundedValue = std::min(
		value,
		m_capacity - otherColorCount()
	);

	switch (m_draftConfig.selectedColorChannel) {
	case ColorChannel::Green:
		m_draftConfig.greenCount = boundedValue;
		break;
	case ColorChannel::Blue:
		m_draftConfig.blueCount = boundedValue;
		break;
	case ColorChannel::Red:
	default:
		m_draftConfig.redCount = boundedValue;
		break;
	}
}

unsigned int ParticleSimWorkspace::requestedParticleCount() const {
	if (m_draftConfig.colorMode == ColorMode::Default) {
		return m_draftConfig.defaultParticleCount;
	}

	return m_draftConfig.redCount +
		m_draftConfig.greenCount +
		m_draftConfig.blueCount;
}

int ParticleSimWorkspace::radiusPresetIndex(float radius) {
	int closest = 0;
	float closestDistance = std::fabs(radius - kRadiusPresets[0]);
	for (int i = 1; i < kRadiusPresetCount; ++i) {
		const float distance = std::fabs(radius - kRadiusPresets[i]);
		if (distance < closestDistance) {
			closest = i;
			closestDistance = distance;
		}
	}
	return closest;
}

float ParticleSimWorkspace::radiusPreset(int index) {
	return kRadiusPresets[std::clamp(index, 0, kRadiusPresetCount - 1)];
}

string ParticleSimWorkspace::radiusText(float radius) {
	ostringstream stream;
	stream << fixed << setprecision(4) << radius;
	return stream.str();
}

string ParticleSimWorkspace::voxelText(unsigned int voxelId) {
	ostringstream stream;
	stream << "VOXEL_" << setw(3) << setfill('0') << voxelId;
	return stream.str();
}

const char* ParticleSimWorkspace::gridLayoutName() const {
	switch (m_draftConfig.gridLayout) {
	case GridLayout::MajorGrid: return "MAJOR_GRID";
	case GridLayout::Dynamic: return "DYNAMIC";
	case GridLayout::None:
	default:
		return "NONE";
	}
}

const char* ParticleSimWorkspace::colorModeName() const {
	return m_draftConfig.colorMode == ColorMode::RGB ? "RGB" : "DEFAULT";
}

const char* ParticleSimWorkspace::radiusModeName() const {
	return m_draftConfig.radiusMode == RadiusMode::Random
		? "RANDOM"
		: "UNIFORM";
}

const char* ParticleSimWorkspace::colorChannelName() const {
	switch (m_draftConfig.selectedColorChannel) {
	case ColorChannel::Green: return "GREEN";
	case ColorChannel::Blue: return "BLUE";
	case ColorChannel::Red:
	default:
		return "RED";
	}
}

const char* ParticleSimWorkspace::resetModeName() const {
	return m_draftConfig.resetMode == ResetMode::Random
		? "RANDOM"
		: "DEFAULT";
}
