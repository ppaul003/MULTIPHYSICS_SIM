#ifndef NDMSM_WORKSPACE_PRESENTATION_EM_H
#define NDMSM_WORKSPACE_PRESENTATION_EM_H

#include <string>
#include <vector>

enum class WorkspaceStatusTone {
    Neutral = 0,
    Ready,
    Warning,
    Transition
};

struct WorkspacePanelRow {
    std::string label;
    std::string value;
    bool selectable = false;
    bool selected = false;
};

struct WorkspacePanelSection {
    std::string heading;
    std::vector<WorkspacePanelRow> rows;
};

struct WorkspacePresentation {
    bool panelVisible = false;
    bool statusBlink = false;
    bool frameBlink = false;

    WorkspaceStatusTone frameTone = WorkspaceStatusTone::Neutral;
    WorkspaceStatusTone statusTone = WorkspaceStatusTone::Neutral;

    std::string workspaceName;
    std::string layerLabel;
    std::string statusLine;

    std::vector<WorkspacePanelSection> sections;
    std::vector<std::string> postStatusLines;

    std::string footerLine1;
    std::string footerLine2;
};

#endif
