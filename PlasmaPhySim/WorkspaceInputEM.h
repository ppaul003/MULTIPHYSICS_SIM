#ifndef NDMSM_WORKSPACE_INPUT_EM_H
#define NDMSM_WORKSPACE_INPUT_EM_H

enum class WorkspaceInputAction {
    None = 0,
    Previous,
    Next,
    Decrease,
    Increase,
    Activate,
    Back,
    RawKey,
    TogglePanel,
    TogglePause
};

struct WorkspaceInputEvent {
    WorkspaceInputAction action = WorkspaceInputAction::None;
    unsigned char rawKey = 0;
    int x = 0;
    int y = 0;
};

#endif
