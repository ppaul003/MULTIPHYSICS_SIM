#include "ViewPortEM.h"

#include <chrono>
#include <cmath>
#include <string>

void ViewPort::resize(int width, int height) {
    m_windowWidth = clampPositive(width);
    m_windowHeight = clampPositive(height);
    glViewport(0, 0, m_windowWidth, m_windowHeight);
}

void ViewPort::applyPerspective(float fovDegrees) {
    m_fov = fovDegrees;
    glViewport(0, 0, m_windowWidth, m_windowHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(m_fov, getAspect(), 0.01, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

float ViewPort::getAspect() const {
    if (m_windowHeight <= 0) return 1.0f;
    return static_cast<float>(m_windowWidth) /
        static_cast<float>(m_windowHeight);
}

void ViewPort::beginOverlay2D() {
    glViewport(0, 0, m_windowWidth, m_windowHeight);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, static_cast<double>(m_windowWidth),
        static_cast<double>(m_windowHeight), 0.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void ViewPort::endOverlay2D() {
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void ViewPort::drawText2D(float x, float y, const char* text, void* font) {
    if (!text) return;
    glRasterPos2f(x, y);
    for (const char* p = text; *p; ++p)
        glutBitmapCharacter(font, *p);
}

void ViewPort::updatePanelAnimation(bool visible) {
    const float target = visible ? 1.0f : 0.0f;
    m_panelSlide += (target - m_panelSlide) * 0.15f;
    if (m_panelSlide < 0.001f) m_panelSlide = 0.0f;
    if (m_panelSlide > 0.999f) m_panelSlide = 1.0f;
}

float ViewPort::panelOffsetX() const {
    const float hiddenX = -(m_panelWidth + m_margin + 24.0f);
    return hiddenX * (1.0f - m_panelSlide);
}

void ViewPort::drawWorkspaceFrame(const WorkspacePresentation& p) {
    const float margin = 24.0f;
    const float x0 = margin;
    const float y0 = margin;
    const float x1 = static_cast<float>(m_windowWidth) - margin;
    const float y1 = static_cast<float>(m_windowHeight) - margin;

    float r = 0.85f, g = 0.95f, b = 1.00f, alpha = 0.30f;

    if (p.frameTone == WorkspaceStatusTone::Transition) {
        r = 1.00f; g = 0.65f; b = 0.15f; alpha = 0.95f;
    }
    else if (p.frameTone == WorkspaceStatusTone::Ready) {
        r = 0.45f; g = 0.95f; b = 1.00f; alpha = 0.85f;
    }

    if (p.frameBlink) {
        using Clock = std::chrono::steady_clock;
        const float seconds = std::chrono::duration<float>(
            Clock::now().time_since_epoch()).count();
        if (std::fmod(seconds, 0.50f) >= 0.25f) alpha *= 0.20f;
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glUseProgram(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glLineWidth(1.0f);
    glColor4f(r, g, b, alpha);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x0, y0);
    glVertex2f(x1, y0);
    glVertex2f(x1, y1);
    glVertex2f(x0, y1);
    glEnd();
}

void ViewPort::drawPanelBackground() {
    const float x0 = panelX(m_margin);
    const float y0 = m_margin;
    const float x1 = panelX(m_panelWidth);
    const float y1 = static_cast<float>(m_windowHeight) - m_margin;

    glColor4f(0.02f, 0.04f, 0.06f, 0.76f * m_panelSlide);
    glBegin(GL_QUADS);
    glVertex2f(x0, y0); glVertex2f(x1, y0);
    glVertex2f(x1, y1); glVertex2f(x0, y1);
    glEnd();

    glLineWidth(1.5f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.95f * m_panelSlide);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x0, y0); glVertex2f(x1, y0);
    glVertex2f(x1, y1); glVertex2f(x0, y1);
    glEnd();
    glLineWidth(1.0f);
}

void ViewPort::drawHeader(const WorkspacePresentation& p) {
    const float x = panelX(95.0f);

    glColor4f(
        1.0f,
        1.0f,
        1.0f,
        m_panelSlide
    );

    drawText2D(
        x,
        100.0f,
        "ANAHEIM SYSTEMS DYNAMICS",
        GLUT_BITMAP_HELVETICA_18
    );

    drawText2D(
        x,
        128.0f,
        "Neural-Net Drive Multiphysics Modeling & Simulation",
        GLUT_BITMAP_HELVETICA_18
    );

    drawText2D(
        x,
        156.0f,
        "System Software Ver 0.0.0",
        GLUT_BITMAP_HELVETICA_18
    );

    // ----------------------------------------
    // Workspace / Layer information
    // ----------------------------------------
    if (!p.workspaceName.empty()) {
        glColor4f(
            0.82f,
            0.86f,
            0.90f,
            m_panelSlide
        );

        drawText2D(
            x,
            190.0f,
            p.workspaceName.c_str(),
            GLUT_BITMAP_HELVETICA_18
        );
    }

    // ----------------------------------------
    // Divider
    // ----------------------------------------
    glColor4f(
        0.55f,
        0.60f,
        0.65f,
        0.65f * m_panelSlide
    );

    glLineWidth(1.0f);

    glBegin(GL_LINES);
    glVertex2f(x, 210.0f);
    glVertex2f(panelX(505.0f), 210.0f);
    glEnd();

    // ----------------------------------------
    // Optional layer label
    // ----------------------------------------
    if (!p.layerLabel.empty()) {
        glColor4f(
            0.72f,
            0.78f,
            0.82f,
            m_panelSlide
        );

        drawText2D(
            x,
            232.0f,
            p.layerLabel.c_str(),
            GLUT_BITMAP_HELVETICA_18
        );
    }
}

void ViewPort::drawSections(const WorkspacePresentation& p) {
    float y = 290.0f;
    const float x = 95.0f;

    for (const WorkspacePanelSection& section : p.sections) {
        if (!section.heading.empty()) {
            glColor4f(0.85f, 0.95f, 1.0f, m_panelSlide);
            drawText2D(panelX(x), y, section.heading.c_str(), GLUT_BITMAP_HELVETICA_18);
            y += 42.0f;
        }

        for (const WorkspacePanelRow& row : section.rows) {
            if (row.selected) {
                glColor4f(0.45f, 1.0f, 0.65f, m_panelSlide);
                drawText2D(panelX(x - 28.0f), y, ">", GLUT_BITMAP_HELVETICA_18);
            }
            else {
                glColor4f(0.72f, 0.78f, 0.82f, m_panelSlide);
            }

            std::string line = row.label;
            if (!row.value.empty()) {
                line += " { ";
                line += row.value;
                line += " }";
            }
            drawText2D(panelX(x), y, line.c_str(), GLUT_BITMAP_HELVETICA_18);
            y += m_rowSpacing;
        }
    }

    if (!p.statusLine.empty()) {
        y += 12.0f;
        float alpha = m_panelSlide;

        if (p.statusBlink) {
            using Clock = std::chrono::steady_clock;
            const float seconds = std::chrono::duration<float>(
                Clock::now().time_since_epoch()).count();
            if (std::fmod(seconds, 0.50f) >= 0.25f) alpha *= 0.20f;
        }

        switch (p.statusTone) {
        case WorkspaceStatusTone::Ready:
            glColor4f(0.45f, 1.0f, 0.65f, alpha);
            break;
        case WorkspaceStatusTone::Warning:
            glColor4f(1.0f, 0.45f, 0.45f, alpha);
            break;
        case WorkspaceStatusTone::Transition:
            glColor4f(1.0f, 0.65f, 0.15f, alpha);
            break;
        default:
            glColor4f(0.72f, 0.78f, 0.82f, alpha);
            break;
        }

        drawText2D(panelX(x), y, p.statusLine.c_str(), GLUT_BITMAP_HELVETICA_18);
    }
}

void ViewPort::drawFooter(const WorkspacePresentation& p) {
    const float x = panelX(95.0f);

    // ----------------------------------------
    // Divider
    // ----------------------------------------
    glColor4f(
        0.45f,
        0.50f,
        0.55f,
        0.55f * m_panelSlide
    );

    glBegin(GL_LINES);
    glVertex2f(x, 790.0f);
    glVertex2f(panelX(505.0f), 790.0f);
    glEnd();

    // ----------------------------------------
    // Existing footer controls
    // ----------------------------------------
    if (!p.footerLine1.empty()) {
        glColor4f(
            0.70f,
            0.72f,
            0.75f,
            m_panelSlide
        );

        drawText2D(
            x,
            820.0f,
            p.footerLine1.c_str(),
            GLUT_BITMAP_HELVETICA_12
        );
    }

    if (!p.footerLine2.empty()) {
        drawText2D(
            x,
            850.0f,
            p.footerLine2.c_str(),
            GLUT_BITMAP_HELVETICA_12
        );
    }
}

void ViewPort::drawOverlay(const WorkspacePresentation& p) {
    updatePanelAnimation(p.panelVisible);

    beginOverlay2D();
    drawWorkspaceFrame(p);

    if (m_panelSlide > 0.0f) {
        drawPanelBackground();
        drawHeader(p);
        drawSections(p);
        drawFooter(p);
    }

    endOverlay2D();
}
