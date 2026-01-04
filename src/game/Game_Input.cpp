#include "JumpShootGame.h"
#include "../engine/Input.h"
#include "../engine/Components.h"
#include <cmath>
#include <SDL2/SDL.h>

using namespace PixelsEngine;

void JumpShootGame::HandleInputGameplay(float dt) {
    if (Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
        m_State = GameState::Paused;
        SDL_SetRelativeMouseMode(SDL_FALSE);
        m_MenuSelection = 0;
        return;
    }

    auto* t = m_Registry.GetComponent<Transform3DComponent>(m_PlayerEntity);
    auto* p = m_Registry.GetComponent<PlayerControlComponent>(m_PlayerEntity);
    auto* phys = m_Registry.GetComponent<PhysicsComponent>(m_PlayerEntity);
    auto* weapon = m_Registry.GetComponent<WeaponComponent>(m_PlayerEntity);
    
    if (!t || !p || !phys || !weapon) return;

    // Mouse Look
    int mx, my;
    Input::GetMouseDelta(mx, my);
    t->rot += mx * p->mouseSensitivity;
    t->pitch -= my * 2.0f; 
    
    // Clamp pitch
    if (t->pitch > 200) t->pitch = 200;
    if (t->pitch < -200) t->pitch = -200;
    
    // Movement
    float moveSpeed = p->speed * dt;
    float dx = 0, dy = 0;
    bool isMoving = false;
    
    if (Input::IsKeyDown(SDL_SCANCODE_W)) {
        dx += cos(t->rot) * moveSpeed;
        dy += sin(t->rot) * moveSpeed;
        isMoving = true;
    }
    if (Input::IsKeyDown(SDL_SCANCODE_S)) {
        dx -= cos(t->rot) * moveSpeed;
        dy -= sin(t->rot) * moveSpeed;
        isMoving = true;
    }
    if (Input::IsKeyDown(SDL_SCANCODE_A)) {
        dx += cos(t->rot - M_PI/2) * moveSpeed;
        dy += sin(t->rot - M_PI/2) * moveSpeed;
        isMoving = true;
    }
    if (Input::IsKeyDown(SDL_SCANCODE_D)) {
        dx += cos(t->rot + M_PI/2) * moveSpeed;
        dy += sin(t->rot + M_PI/2) * moveSpeed;
        isMoving = true;
    }
    
    // Collision Check (Simple Slide)
    if (m_Map.Get(int(t->x + dx * 2), int(t->y)) == 0) t->x += dx;
    if (m_Map.Get(int(t->x), int(t->y + dy * 2)) == 0) t->y += dy;
    
    // View Bobbing logic
    if (isMoving && phys->isGrounded) {
        m_BobTimer += dt * 10.0f;
    } else {
        m_BobTimer = 0;
    }

    // Jump
    if (Input::IsKeyDown(SDL_SCANCODE_SPACE) && (phys->isGrounded || phys->isWallRunning)) {
        phys->velZ = p->jumpForce;
        phys->isGrounded = false;
        phys->isWallRunning = false;
    }
    
    // Shooting
    bool wantsGrapple = Input::IsMouseButtonPressed(SDL_BUTTON_RIGHT) || 
                        (Input::IsKeyDown(SDL_SCANCODE_E) && Input::IsMouseButtonPressed(SDL_BUTTON_LEFT));

    if (Input::IsMouseButtonDown(SDL_BUTTON_LEFT) && !Input::IsKeyDown(SDL_SCANCODE_E)) {
        if (!weapon->isDrawing && weapon->cooldown <= 0) {
            weapon->isDrawing = true;
            weapon->drawTime = 0.0f;
        } else if (weapon->isDrawing) {
            weapon->drawTime += dt;
            if (weapon->drawTime > 1.0f) weapon->drawTime = 1.0f;
        }
    } else if (wantsGrapple && weapon->cooldown <= 0) {
        // Grapple!
        auto arrow = m_Registry.CreateEntity();
        float ax = t->x + cos(t->rot) * 0.5f;
        float ay = t->y + sin(t->rot) * 0.5f;
        m_Registry.AddComponent<Transform3DComponent>(arrow, {ax, ay, t->z, t->rot, t->pitch});
        m_Registry.AddComponent<ProjectileComponent>(arrow, {ProjectileComponent::Grapple, 0.0f, true, 2.0f});
        float speed = 25.0f;
        m_Registry.AddComponent<PhysicsComponent>(arrow, {cos(t->rot)*speed, sin(t->rot)*speed, t->pitch * 0.05f, 0.0f, false, false, 0.0f, 0.0f});
        weapon->cooldown = 1.0f;
    } else {
        if (weapon->isDrawing) {
            // Fire Arrow
            weapon->isDrawing = false;
            weapon->cooldown = 0.5f;
            
            auto arrow = m_Registry.CreateEntity();
            float ax = t->x + cos(t->rot) * 0.5f;
            float ay = t->y + sin(t->rot) * 0.5f;
            
            m_Registry.AddComponent<Transform3DComponent>(arrow, {ax, ay, t->z, t->rot, t->pitch});
            m_Registry.AddComponent<ProjectileComponent>(arrow, {ProjectileComponent::Arrow, 50.0f, true, 5.0f});
            
            float speed = 20.0f * (0.5f + weapon->drawTime); 
            float vz = t->pitch * 0.05f; 
            
            m_Registry.AddComponent<PhysicsComponent>(arrow, {cos(t->rot)*speed, sin(t->rot)*speed, vz, 15.0f, false, false, 0.0f, 0.0f});
            m_Registry.AddComponent<BillboardComponent>(arrow, {m_BowIdle, 0.2f, 0.2f, 0.2f, true}); 
        }
    }
    
    if (weapon->cooldown > 0) weapon->cooldown -= dt;
}

void JumpShootGame::HandleInputMenu() {
    int w, h;
    SDL_GetRendererOutputSize(m_Renderer, &w, &h);
    int mx, my;
    Input::GetMousePosition(mx, my);

    int btnW = 200;
    int btnH = 50;
    int startY = 300;
    int gap = 70;
    int startX = w/2 - btnW/2;
    
    bool action = false;

    // Mouse Interaction
    for (int i = 0; i < 3; i++) {
        int by = startY + gap * i;
        if (mx >= startX && mx <= startX + btnW && my >= by && my <= by + btnH) {
            if (m_MenuSelection != i) m_MenuSelection = i;
            
            if (Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
                action = true;
            }
        }
    }

    // Keyboard Interaction
    if (Input::IsKeyPressed(SDL_SCANCODE_W) || Input::IsKeyPressed(SDL_SCANCODE_UP)) {
        m_MenuSelection--;
        if (m_MenuSelection < 0) m_MenuSelection = 2;
    }
    if (Input::IsKeyPressed(SDL_SCANCODE_S) || Input::IsKeyPressed(SDL_SCANCODE_DOWN)) {
        m_MenuSelection++;
        if (m_MenuSelection > 2) m_MenuSelection = 0;
    }
    
    if (Input::IsKeyPressed(SDL_SCANCODE_RETURN) || Input::IsKeyPressed(SDL_SCANCODE_SPACE)) {
        action = true;
    }
    
    if (action) {
        if (m_MenuSelection == 0) { // Play
            InitGame();
            m_State = GameState::Playing;
            SDL_SetRelativeMouseMode(SDL_TRUE);
        } else if (m_MenuSelection == 1) { // Options
            // TODO
        } else if (m_MenuSelection == 2) { // Quit
            m_IsRunning = false;
        }
    }
}

void JumpShootGame::HandleInputPause() {
    if (Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
        m_State = GameState::Playing;
        SDL_SetRelativeMouseMode(SDL_TRUE);
        return;
    }

    int w, h;
    SDL_GetRendererOutputSize(m_Renderer, &w, &h);
    int mx, my;
    Input::GetMousePosition(mx, my);

    int btnW = 200;
    int btnH = 50;
    int startY = 250;
    int gap = 70;
    int startX = w/2 - btnW/2;
    
    bool action = false;

    // Mouse Interaction
    for (int i = 0; i < 4; i++) {
        int by = startY + gap * i;
        if (mx >= startX && mx <= startX + btnW && my >= by && my <= by + btnH) {
             if (m_MenuSelection != i) m_MenuSelection = i;

             if (Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
                action = true;
             }
        }
    }

    // Keyboard Interaction
    if (Input::IsKeyPressed(SDL_SCANCODE_W) || Input::IsKeyPressed(SDL_SCANCODE_UP)) {
        m_MenuSelection--;
        if (m_MenuSelection < 0) m_MenuSelection = 3;
    }
    if (Input::IsKeyPressed(SDL_SCANCODE_S) || Input::IsKeyPressed(SDL_SCANCODE_DOWN)) {
        m_MenuSelection++;
        if (m_MenuSelection > 3) m_MenuSelection = 0;
    }
    
    if (Input::IsKeyPressed(SDL_SCANCODE_RETURN) || Input::IsKeyPressed(SDL_SCANCODE_SPACE)) {
        action = true;
    }
    
    if (action) {
        if (m_MenuSelection == 0) { // Resume
            m_State = GameState::Playing;
            SDL_SetRelativeMouseMode(SDL_TRUE);
        } else if (m_MenuSelection == 1) { // Save
            // TODO
        } else if (m_MenuSelection == 2) { // Main Menu
             m_State = GameState::MainMenu;
             m_MenuSelection = 0;
        } else if (m_MenuSelection == 3) { // Quit
            m_IsRunning = false;
        }
    }
}
