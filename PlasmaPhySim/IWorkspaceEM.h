#ifndef NDMSM_IWORKSPACE_EM_H
#define NDMSM_IWORKSPACE_EM_H

#include "WorkspaceContextEM.h"
#include "WorkspaceInputEM.h"
#include "WorkspacePresentationEM.h"

class IWorkspace {
public:
    virtual ~IWorkspace() = default;

    virtual bool initialize(WorkspaceServices& services) = 0;
    virtual void enter(WorkspaceServices& services) = 0;
    virtual void exit(WorkspaceServices& services) = 0;

    virtual void update(
        const WorkspaceFrameContext& frame,
        WorkspaceServices& services) = 0;

    virtual void render(
        const WorkspaceFrameContext& frame,
        WorkspaceServices& services) = 0;

    virtual bool handleInput(
        const WorkspaceInputEvent& input,
        WorkspaceServices& services) = 0;

    virtual WorkspacePresentation buildPresentation() const = 0;
};

#endif
