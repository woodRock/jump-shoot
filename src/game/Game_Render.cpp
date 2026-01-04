#include "JumpShootGame.h"
#include <cmath>
#include "../engine/Components.h"

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
        // Apply Bobbing to Camera z temporarily for render
        Camera bobCam = *m_Camera;
        float bobOffset = sin(m_BobTimer) * 0.05f;
        bobCam.z += bobOffset;
        
        // Wall Run tilt
        auto* phys = m_Registry.GetComponent<PhysicsComponent>(m_PlayerEntity);
        // We'd need a roll/tilt in Camera struct for real tilt, 
        // but we can fake it with a small yaw/pitch offset or just leave it.
        // Let's just do the bobbing for now.

        m_Raycaster.Render(m_Renderer, bobCam, m_Map, m_Registry);
        RenderUI();
    } else if (m_State == GameState::Paused) {
        m_Raycaster.Render(m_Renderer, *m_Camera, m_Map, m_Registry);
        RenderUI(); // Optional: Hide UI behind pause?
        RenderPauseMenu();
    }
}