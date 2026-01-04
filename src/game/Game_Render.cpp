#include "JumpShootGame.h"
#include <cmath>

using namespace PixelsEngine;

void JumpShootGame::OnRender() {
    if (m_State == GameState::MainMenu) {
        // Draw 3D Background
        // Use a rotating camera
        Camera menuCam = *m_Camera;
        menuCam.x = 12.0f + cos(m_MenuCamAngle) * 5.0f;
        menuCam.y = 12.0f + sin(m_MenuCamAngle) * 5.0f;
        menuCam.z = 8.0f; // High up
        menuCam.yaw = m_MenuCamAngle + M_PI; // Look at center (12,12)
        menuCam.pitch = -50.0f; // Look down
        
        m_Raycaster.Render(m_Renderer, menuCam, m_Map, m_Registry);
        
        RenderMainMenu();
    } else if (m_State == GameState::Playing) {
        m_Raycaster.Render(m_Renderer, *m_Camera, m_Map, m_Registry);
        RenderUI();
    } else if (m_State == GameState::Paused) {
        m_Raycaster.Render(m_Renderer, *m_Camera, m_Map, m_Registry);
        RenderUI(); // Optional: Hide UI behind pause?
        RenderPauseMenu();
    }
}