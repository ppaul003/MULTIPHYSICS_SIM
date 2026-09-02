#include "TheArbiterEM.h"

TheArbiter::TheArbiter() = default;

TheArbiter::ArbiterResult
TheArbiter::routeKeyboard(const KeyboardInput::KeyEvent& event) const {
    ArbiterResult result;
    result.workspaceInput.x = event.x;
    result.workspaceInput.y = event.y;

    if (event.signal == KeyboardInput::KEY_ESCAPE) {
        result.handled = true;
        result.arbiterCommand = ArbiterCommand::CMD_EXIT;
        return result;
    }

    switch (event.signal) {
    case KeyboardInput::KEY_W:
        result.workspaceInput.action = WorkspaceInputAction::Previous;
        break;
    case KeyboardInput::KEY_S:
        result.workspaceInput.action = WorkspaceInputAction::Next;
        break;
    case KeyboardInput::KEY_A:
        result.workspaceInput.action = WorkspaceInputAction::Decrease;
        break;
    case KeyboardInput::KEY_D:
        result.workspaceInput.action = WorkspaceInputAction::Increase;
        break;
    case KeyboardInput::KEY_E:
    case KeyboardInput::KEY_ENTER:
        result.workspaceInput.action = WorkspaceInputAction::Activate;
        break;
    case KeyboardInput::KEY_Q:
        result.workspaceInput.action = WorkspaceInputAction::Back;
        break;
    default:
        result.workspaceInput.action = WorkspaceInputAction::None;
        break;
    }

    if (result.workspaceInput.action != WorkspaceInputAction::None) {
        result.hasWorkspaceInput = true;
        result.handled = true;
    }

    return result;
}

void TheArbiter::requestEnterDomain(WorkspaceDomain domain) {
    m_navigationRequest.type = NavigationRequestType::ENTER_DOMAIN;
    m_navigationRequest.domain = domain;
}

void TheArbiter::requestReturnToGlobalShell(WorkspaceDomain domain) {
    m_navigationRequest.type = NavigationRequestType::RETURN_GLOBAL_SHELL;
    m_navigationRequest.domain = domain;
}

TheArbiter::NavigationRequest TheArbiter::takeNavigationRequest() {
    const NavigationRequest request = m_navigationRequest;
    m_navigationRequest = NavigationRequest{};
    return request;
}

bool TheArbiter::isGlobalShell() const {
    return m_navigation.layer == ApplicationLayer::GLOBAL_SHELL;
}

bool TheArbiter::isDomainSelection() const {
    return m_navigation.layer == ApplicationLayer::DOMAIN_SELECTION;
}
