#include "../engine/Components.h"
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
    menuCam.z = 8.0f;                    // High up
    menuCam.yaw = m_MenuCamAngle + M_PI; // Look at center (12,12)
    menuCam.pitch = -50.0f;              // Look down

    m_Raycaster.Render(m_Renderer, menuCam, m_Map, m_Registry, 0.0f);

    RenderMainMenu();
  } else if (m_State == GameState::Playing) {
    int w = m_Width;
    int h = m_Height;

    // Apply Bobbing to Camera z temporarily for render
    Camera bobCam = *m_Camera;
    float bobOffset = sin(m_BobTimer) * 0.05f;
    bobCam.z += bobOffset;

    m_Raycaster.Render(m_Renderer, bobCam, m_Map, m_Registry, m_CameraRoll);

    // Render Grapple Rope
    if (m_IsGrappling) {
      SDL_SetRenderDrawColor(m_Renderer, 150, 150, 150, 255);
      SDL_RenderDrawLine(m_Renderer, w / 2, h, w / 2, h / 2);
    }

    // Slow-mo Visual Cue (Ghosting)
    if (m_TimeScale < 1.0f) {
      SDL_SetRenderDrawBlendMode(m_Renderer, SDL_BLENDMODE_BLEND);
      // Outer vignette for slow-mo
      SDL_SetRenderDrawColor(m_Renderer, 0, 50, 150, 40);
      SDL_Rect screenRect = {0, 0, w, h};
      SDL_RenderFillRect(m_Renderer, &screenRect);

      // Central focus
      SDL_SetRenderDrawColor(m_Renderer, 255, 255, 255, 10);
      SDL_Rect focus = {w / 4, h / 4, w / 2, h / 2};
      SDL_RenderFillRect(m_Renderer, &focus);

      SDL_SetRenderDrawBlendMode(m_Renderer, SDL_BLENDMODE_NONE);
    }
    RenderUI();
  } else if (m_State == GameState::Paused) {
    m_Raycaster.Render(m_Renderer, *m_Camera, m_Map, m_Registry, m_CameraRoll);
    RenderUI(); // Optional: Hide UI behind pause?
    RenderPauseMenu();
  }
}
