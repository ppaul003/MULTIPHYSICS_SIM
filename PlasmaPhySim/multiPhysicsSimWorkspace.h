#ifndef NDMSM_MULTIPHYSICS_SIM_WORKSPACE_H
#define NDMSM_MULTIPHYSICS_SIM_WORKSPACE_H

#include "IWorkspaceEM.h"
#include "TheArbiterEM.h"

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
    WorkspacePresentation buildLayer1TransitionPresentation() const;

private:
    WorkspacePresentation buildLayer1Presentation() const;
    void renderLayer1DomainBoundary(WorkspaceServices& services) const;

private:
    TheArbiter* m_arbiter = nullptr; // non-owning
    bool m_active = false;
};

#endif
