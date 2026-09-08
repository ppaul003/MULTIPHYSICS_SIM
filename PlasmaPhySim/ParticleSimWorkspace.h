#ifndef VITRUGEN_PARTICLE_SIM_WORKSPACE_H
#define VITRUGEN_PARTICLE_SIM_WORKSPACE_H

#include <memory>
#include <string>
#include <vector>

#include <vector_types.h>
#include <vector_functions.h>
#include <cuda_runtime.h>

#include "TheArbiterEM.h"
#include "IWorkspaceEM.h"
#include "particleSystem.h"

class ParticleSimWorkspace : public IWorkspace {
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
	WorkspacePresentation buildLayer1TransitionPresentation() const;


private:
	enum class Layer1Row {
		WorkspaceSelection = 0,
		ParticleMode,
		GridLayout,
		Configure,
		Count
	};

	enum class ParticleMode {
		Baseline = 0,
		Electromagnetics
	};

	enum class GridLayout {
		None = 0,
		Minimal,
		Full,
		Dynamic
	};

	enum class ColorMode {
		Default = 0,
		RGB
	};

	enum class RadiusMode {
		Uniform = 0,
		Random
	};

	enum class ColorChannel {
		Red = 0,
		Green,
		Blue
	};

	enum class ResetMode {
		Default = 0,
		Random
	};

	struct DraftConfig {

		GridLayout gridLayout =
			GridLayout::Full;

		ColorMode colorMode =
			ColorMode::Default;

		RadiusMode radiusMode =
			RadiusMode::Uniform;

		unsigned int defaultParticleCount = 4200;

		unsigned int redCount = 0;
		unsigned int greenCount = 0;
		unsigned int blueCount = 0;

		ColorChannel selectedColorChannel =
			ColorChannel::Red;

		ResetMode resetMode =
			ResetMode::Default;

		float uniformRadius = 0.0120f;
		float minimumRadius = 0.0098f;
		float maximumRadius = 0.0156f;
	};

	struct RuntimeConfig {

		unsigned int capacity = 0;
		unsigned int activeCount = 0;

		unsigned int redCount = 0;
		unsigned int greenCount = 0;
		unsigned int blueCount = 0;

		float uniformRadius = 0.0120f;
		float minimumRadius = 0.0098f;
		float maximumRadius = 0.0156f;
		float placementRadius = 0.0120f;

		ColorMode colorMode =
			ColorMode::Default;

		RadiusMode radiusMode =
			RadiusMode::Uniform;

		ResetMode resetMode =
			ResetMode::Default;
	};
	
	WorkspacePresentation buildLayer1Presentation() const;
	//WorkspacePresentation buildLayer2Presentation() const;
	//WorkspacePresentation buildLayer3Presentation() const;

	void renderLayer1DomainBoundary(WorkspaceServices& services) const;
	bool resolveRuntimeConfig(RuntimeConfig& resolved) const;
	bool applyRuntimeConfig();
	void moveCursor(int direction);
	void adjustSelectedValue(int direction, WorkspaceServices& services);
	const char* particleModeName() const;
	const char* gridLayoutName() const;
	

private:
	static constexpr float kMaximumSupportedRadius = 0.0156f;
	static constexpr unsigned int kParticleCapacity = 16384;
	static constexpr unsigned int kGridSize = 64;


	unsigned int m_capacity = kParticleCapacity;
	unsigned int m_activeCount = kParticleCapacity;

	uint3 m_gridDimensions = make_uint3(kGridSize, kGridSize, kGridSize);

	std::unique_ptr<ParticleSystem> m_particleSystem;
	std::vector<float> m_radii;

	TheArbiter* m_arbiter = nullptr;
	DraftConfig m_draftConfig;
	RuntimeConfig m_runtimeConfig;
	Layer1Row m_activeRow = Layer1Row::WorkspaceSelection;
	ParticleMode m_particleMode = ParticleMode::Baseline;
	std::string m_statusLine = "READY: CUDA PARTICLE BASELINE.";
	WorkspaceStatusTone m_statusTone = WorkspaceStatusTone::Ready;

	bool m_initialized = false;
	bool m_active = false;
	bool m_paused = true;

	float m_elapsedSimulationTime = 0.0f;
};

#endif
