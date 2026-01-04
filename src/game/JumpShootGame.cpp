#include "JumpShootGame.h"
#include "../engine/Input.h"
#include "../engine/Components.h"

using namespace PixelsEngine;

// Lifecycle methods (ctor/dtor/OnStart) are in Game_Init.cpp

void JumpShootGame::OnUpdate(float deltaTime) {
    if (m_State == GameState::MainMenu) {
        m_MenuCamAngle += 0.2f * deltaTime;
        HandleInputMenu();
    } 
    else if (m_State == GameState::Playing) {
        HandleInputGameplay(deltaTime);
        UpdatePhysics(deltaTime);
        UpdateProjectiles(deltaTime);
        
        // Sync Camera to Player
        auto* t = m_Registry.GetComponent<Transform3DComponent>(m_PlayerEntity);
        if (t) {
            m_Camera->x = t->x;
            m_Camera->y = t->y;
            m_Camera->z = t->z;
            m_Camera->yaw = t->rot;
            m_Camera->pitch = t->pitch;
        }
    } 
    else if (m_State == GameState::Paused) {
        HandleInputPause();
    }
}