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
        auto* phys = m_Registry.GetComponent<PhysicsComponent>(m_PlayerEntity);
        auto* weapon = m_Registry.GetComponent<WeaponComponent>(m_PlayerEntity);
        
        m_TimeScale = 1.0f;
        if (phys && !phys->isGrounded && weapon && weapon->isDrawing) {
            m_TimeScale = 0.3f; // Aero-Focus
        }
        
        float dt = deltaTime * m_TimeScale;
        
        HandleInputGameplay(dt);
        UpdatePhysics(dt);
        UpdateProjectiles(dt);
        
        // View Bobbing
        auto* p = m_Registry.GetComponent<PlayerControlComponent>(m_PlayerEntity);
        auto* t = m_Registry.GetComponent<Transform3DComponent>(m_PlayerEntity);
        if (phys && phys->isGrounded && p && t) {
            // Only bob if moving
            // We'll approximate moving by checking input in handle? 
            // Better: just check if we have any velocity in X/Y 
            // (Wait, we don't store velocity in X/Y for player yet in a way that's easy to check here without physics rework)
            // Let's just bob if W/A/S/D is down (approximated)
        }

        // Sync Camera to Player
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