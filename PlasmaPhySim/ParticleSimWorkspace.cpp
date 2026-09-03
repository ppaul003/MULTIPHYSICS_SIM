#include "ParticleSimWorkspace.h"

#include "IWorkspaceEM.h"

#include "rendererEM_Euclid.h"
#include "TheArbiterEM.h"

#include <algorithm>
#include <cmath>

using namespace std;

bool ParticleSimWorkspace::initialize(
	WorkspaceServices& services) {

	if (m_initialized) return true;

	if (!services.renderer ||
		!services.arbiter) return false;

	m_radii.assign(kParticleCapacity, 0.0f);
	
	m_particleSystem = std::make_unique<ParticleSystem>(
		kParticleCapacity,
		m_gridDimensions,
		true);

	m_particleSystem->setSimulationDomain(4.0f);
	m_particleSystem->setDefaultColorRamp();
	m_particleSystem->reset(ParticleSystem::CNFG_DEFAULT_RESTART);

	m_initialized = true;
	return applyRuntimeConfig();
}

void ParticleSimWorkspace::enter(
	WorkspaceServices& services) {

	if (!m_initialized) return;

	m_active = true;
	m_paused = m_particleMode != ParticleMode::Baseline;
}

void ParticleSimWorkspace::exit(
	WorkspaceServices& services) {

	m_active = false;
}

void ParticleSimWorkspace::update(
	const WorkspaceFrameContext& frame,
	WorkspaceServices& services) {

	(void)services;
	if (!m_active || !m_particleSystem || m_paused) return;

	m_particleSystem->update(frame.deltaTime);
	m_elapsedSimulationTime += frame.deltaTime;
}

void ParticleSimWorkspace::render(
	const WorkspaceFrameContext& frame,
	WorkspaceServices& services) {

	(void)frame;
	if (!m_active || !m_particleSystem || !services.renderer) return;

	EuclidRenderer& renderer = *services.renderer;

	if (m_draftConfig.gridLayout != GridLayout::None) {
		EuclidRenderer::UniformGrid grid;
		const uint3 particleGrid = m_particleSystem->getGridSize();
		const float3 origin = m_particleSystem->getWorldOrigin();
		const float3 cell = m_particleSystem->getCellSize();

		grid.dimensions = glm::ivec3(
			static_cast<int>(particleGrid.x),
			static_cast<int>(particleGrid.y),
			static_cast<int>(particleGrid.z));
		grid.origin = glm::vec3(origin.x, origin.y, origin.z);
		grid.cellSize = glm::vec3(cell.x, cell.y, cell.z);
		grid.majorEvery = std::max(1, renderer.getGridMajorEvery());

		EuclidRenderer::GridDisplay display;
		display.boundary = true;
		display.majorGrid =
			m_draftConfig.gridLayout == GridLayout::Full ||
			m_draftConfig.gridLayout == GridLayout::Dynamic;
		display.minorGrid = m_draftConfig.gridLayout == GridLayout::Dynamic;
		display.axes = false;

		renderer.drawUniformGrid(grid, display);
		const glm::vec3 extent = glm::vec3(grid.dimensions) * grid.cellSize;
		renderer.drawAxisGizmo(
			grid.origin,
			0.20f * std::max({ extent.x, extent.y, extent.z }));
	}

	const unsigned int activeCount = m_particleSystem->getActiveParticleCount();
	if (activeCount == 0) return;

	renderer.setParticleSystem(m_particleSystem.get());
	if (m_radii.size() >= activeCount)
		renderer.setRadius(m_radii.data(), static_cast<int>(activeCount));
	renderer.setVertexBuffer(
		m_particleSystem->getCurrentReadBuffer(),
		static_cast<int>(activeCount));
	renderer.setColorBuffer(m_particleSystem->getColorBuffer());
	renderer.display(EuclidRenderer::PARTICLE_SPHERES);
}

bool ParticleSimWorkspace::handleInput(
	const WorkspaceInputEvent& input,
	WorkspaceServices& services) {

	if (!m_active)
		return false;

	switch (input.action) {
	case WorkspaceInputAction::Previous:
		moveCursor(-1);
		return true;
	case WorkspaceInputAction::Next:
		moveCursor(+1);
		return true;
	case WorkspaceInputAction::Decrease:
		adjustSelectedValue(-1, services);
		return true;
	case WorkspaceInputAction::Increase:
		adjustSelectedValue(+1, services);
		return true;
	case WorkspaceInputAction::Activate:
		if (m_activeRow == Layer1Row::WorkspaceSelection) {
			adjustSelectedValue(+1, services);
		}
		else if (m_activeRow == Layer1Row::Configure) {
			if (m_particleMode == ParticleMode::Electromagnetics) {
				m_paused = true;
				m_statusLine = "ELECTROMAGNETICS is reserved for a future pass.";
				m_statusTone = WorkspaceStatusTone::Warning;
			}
			else if (applyRuntimeConfig()) {
				m_paused = false;
				m_elapsedSimulationTime = 0.0f;
				m_statusLine = "READY: CUDA PARTICLE BASELINE CONFIGURED.";
				m_statusTone = WorkspaceStatusTone::Ready;
			}
			else {
				m_paused = true;
				m_statusLine = "PARTICLE CONFIGURATION IS INVALID.";
				m_statusTone = WorkspaceStatusTone::Warning;
			}
		}
		return true;
	case WorkspaceInputAction::Back:
		if (services.arbiter)
			services.arbiter->requestReturnToGlobalShell(
				TheArbiter::WorkspaceDomain::MULPHY_SIM);
		return true;
	default:
		return false;
	}
}

WorkspacePresentation
ParticleSimWorkspace::buildPresentation() const {

	WorkspacePresentation presentation;
	presentation.panelVisible = true;

	presentation.workspaceName =
		"LAYER 1 -> MULPHY_SIM WORKSPACE CONFIGURATION";

	presentation.layerLabel =
		"MODE: PARTICLE_SIM";

	WorkspacePanelSection section;

	WorkspacePanelRow workspaceRow;
	workspaceRow.label = "[1]: MULPHY_SIM SELECTION";
	workspaceRow.value = "PARTICLE_SIM";
	workspaceRow.selectable = true;
	workspaceRow.selected = m_activeRow == Layer1Row::WorkspaceSelection;
	section.rows.push_back(workspaceRow);

	WorkspacePanelRow modeRow;
	modeRow.label = "[2]: PARTICLE MODE";
	modeRow.value = particleModeName();
	modeRow.selectable = true;
	modeRow.selected = m_activeRow == Layer1Row::ParticleMode;
	section.rows.push_back(modeRow);

	WorkspacePanelRow gridRow;
	gridRow.label = "[3]: GRID LAYOUT";
	gridRow.value = gridLayoutName();
	gridRow.selectable = true;
	gridRow.selected = m_activeRow == Layer1Row::GridLayout;
	section.rows.push_back(gridRow);

	WorkspacePanelRow configureRow;
	configureRow.label = "[4]: PRESS E TO CONFIGURE SIM";
	configureRow.selectable = true;
	configureRow.selected = m_activeRow == Layer1Row::Configure;
	section.rows.push_back(configureRow);

	presentation.sections.push_back(section);
	presentation.statusLine = m_statusLine;
	presentation.statusTone = m_statusTone;
	presentation.footerLine1 = "W/S: Select row    A/D: Change value    E: Configure";
	presentation.footerLine2 = "Q: Return to Global Shell    ESC: Exit";


	return presentation;
}

bool ParticleSimWorkspace::applyRuntimeConfig() {
	if (!m_particleSystem) return false;

	RuntimeConfig resolved;
	if (!resolveRuntimeConfig(resolved)) return false;
	if (!m_particleSystem->setActiveParticleCount(resolved.activeCount)) return false;

	bool radiusApplied = false;
	if (resolved.radiusMode == RadiusMode::Uniform) {
		radiusApplied = m_particleSystem->setUniformActiveRadii(
			resolved.uniformRadius);
	}
	else {
		radiusApplied = m_particleSystem->setRandomActiveRadii(
			resolved.minimumRadius,
			resolved.maximumRadius);
	}
	if (!radiusApplied) return false;

	m_particleSystem->reset(
		resolved.resetMode == ResetMode::Random
		? ParticleSystem::CNFG_RANDOM_RESTART
		: ParticleSystem::CNFG_DEFAULT_RESTART);

	if (resolved.colorMode == ColorMode::RGB) {
		if (!m_particleSystem->setRGBParticleCounts(
			resolved.redCount,
			resolved.greenCount,
			resolved.blueCount)) return false;
	}
	else {
		m_particleSystem->setDefaultColorRamp();
	}

	m_runtimeConfig = resolved;
	m_activeCount = resolved.activeCount;
	m_radii.resize(resolved.activeCount);
	if (resolved.activeCount > 0)
		m_particleSystem->dumpRadii(m_radii.data(), resolved.activeCount);
	return true;
}

void ParticleSimWorkspace::moveCursor(int direction) {
	if (direction == 0) return;
	const int count = static_cast<int>(Layer1Row::Count);
	const int current = static_cast<int>(m_activeRow);
	const int step = direction < 0 ? -1 : 1;
	m_activeRow = static_cast<Layer1Row>((current + step + count) % count);
}

void ParticleSimWorkspace::adjustSelectedValue(
	int direction,
	WorkspaceServices& services) {

	if (direction == 0) return;

	switch (m_activeRow) {
	case Layer1Row::WorkspaceSelection:
		if (services.arbiter)
			services.arbiter->setActiveWorkspace(
				TheArbiter::WorkspaceId::MULTIPHYSICS_SIM);
		break;

	case Layer1Row::ParticleMode:
		m_particleMode =
			m_particleMode == ParticleMode::Baseline
			? ParticleMode::Electromagnetics
			: ParticleMode::Baseline;
		if (m_particleMode == ParticleMode::Electromagnetics) {
			m_paused = true;
			m_statusLine = "ELECTROMAGNETICS is reserved for a future pass.";
			m_statusTone = WorkspaceStatusTone::Warning;
		}
		else {
			m_statusLine = "READY: CUDA PARTICLE BASELINE.";
			m_statusTone = WorkspaceStatusTone::Ready;
		}
		break;

	case Layer1Row::GridLayout: {
		const int count = 4;
		const int current = static_cast<int>(m_draftConfig.gridLayout);
		const int step = direction < 0 ? -1 : 1;
		m_draftConfig.gridLayout = static_cast<GridLayout>(
			(current + step + count) % count);
		break;
	}

	case Layer1Row::Configure:
	case Layer1Row::Count:
	default:
		break;
	}
}

const char* ParticleSimWorkspace::particleModeName() const {
	return m_particleMode == ParticleMode::Electromagnetics
		? "ELECTROMAGNETICS"
		: "BASELINE";
}

const char* ParticleSimWorkspace::gridLayoutName() const {
	switch (m_draftConfig.gridLayout) {
	case GridLayout::None: return "NONE";
	case GridLayout::Minimal: return "MINIMAL";
	case GridLayout::Dynamic: return "DYNAMIC";
	case GridLayout::Full:
	default:
		return "FULL";
	}
}

bool ParticleSimWorkspace::resolveRuntimeConfig(
	RuntimeConfig& resolved) const {

	const unsigned long long requestedCount =
		m_draftConfig.colorMode == ColorMode::Default
		? static_cast<unsigned long long>(m_draftConfig.defaultParticleCount)
		: static_cast<unsigned long long>(m_draftConfig.redCount) +
		static_cast<unsigned long long>(m_draftConfig.greenCount) +
		static_cast<unsigned long long>(m_draftConfig.blueCount);

	if (requestedCount > m_capacity)
		return false;

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

	if (m_draftConfig.radiusMode == RadiusMode::Uniform) {
		if (!uniformRadiusValid) return false;
		
	}
	else if (m_draftConfig.radiusMode == RadiusMode::Random) {
		if (!randomRadiusValid) return false;

	}
	else {

		return false;
	}

	resolved.capacity = m_capacity;

	resolved.activeCount =
		static_cast<unsigned int>(requestedCount);

	resolved.colorMode =
		m_draftConfig.colorMode;

	resolved.radiusMode =
		m_draftConfig.radiusMode;

	resolved.resetMode =
		m_draftConfig.resetMode;

	resolved.uniformRadius =
		m_draftConfig.uniformRadius;

	resolved.minimumRadius =
		m_draftConfig.minimumRadius;

	resolved.maximumRadius =
		m_draftConfig.maximumRadius;

	resolved.placementRadius =
		m_draftConfig.radiusMode == RadiusMode::Random
		? m_draftConfig.maximumRadius
		: m_draftConfig.uniformRadius;

	if (m_draftConfig.colorMode == ColorMode::RGB) {

		resolved.redCount = m_draftConfig.redCount;
		resolved.greenCount = m_draftConfig.greenCount;
		resolved.blueCount = m_draftConfig.blueCount;
	}

	return true;
}
