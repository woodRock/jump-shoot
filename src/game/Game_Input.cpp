#include "../engine/Components.h"
#include "../engine/Config.h"
#include "../engine/Input.h"
#include "JumpShootGame.h"
#include <SDL2/SDL.h>
#include <cmath>

using namespace PixelsEngine;

void JumpShootGame::HandleInputGameplay(float dt) {
  // Victory / Game Over Input
  if (m_GameFinished) {
      if (Input::IsKeyPressed(SDL_SCANCODE_SPACE)) {
          // Next Level
          m_CurrentLevel++;
          if (m_CurrentLevel > 3) m_CurrentLevel = 1; // Loop back or end game
          InitGame();
          return;
      }
      if (Input::IsKeyPressed(SDL_SCANCODE_R)) {
          // Retry
          InitGame();
          return;
      }
      // If finished, don't process other gameplay input? 
      // The original code returned. But if we want to look around...
      // Original code returned immediately. Let's keep that behavior or allow looking?
      // "Victory Restart" implies we are just waiting for restart.
      // But maybe we want to fly around?
      // Let's stick to the request: "at the screen where it displays there time at the end of the level".
      // Usually this screen overlays gameplay.
      // If we return here, we can't look around.
      // But the original code returned.
      // I'll keep returning to prevent moving/shooting after finish.
      return; 
  }

  if (Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
    m_State = GameState::Paused;
    SDL_SetRelativeMouseMode(SDL_FALSE);
    m_MenuSelection = 0;
    return;
  }

  if (Input::IsKeyPressed(Config::GetKeybind(GameAction::ToggleFullScreen))) {
    ToggleFullScreen();
  }

  auto *t = m_Registry.GetComponent<Transform3DComponent>(m_PlayerEntity);
  auto *p = m_Registry.GetComponent<PlayerControlComponent>(m_PlayerEntity);
  auto *phys = m_Registry.GetComponent<PhysicsComponent>(m_PlayerEntity);
  auto *weapon = m_Registry.GetComponent<WeaponComponent>(m_PlayerEntity);

  if (!t || !p || !phys || !weapon)
    return;

  // Mouse Look
  int mx, my;
  Input::GetMouseDelta(mx, my);
  float sensitivity = Config::GetMouseSensitivity();
  t->rot += mx * sensitivity;
  t->pitch -= my * 2.0f;

  // Clamp pitch
  if (t->pitch > 400)
    t->pitch = 400;
  if (t->pitch < -400)
    t->pitch = -400;

  // Movement (Acceleration)
  float accel = (phys->isGrounded ? 50.0f : 15.0f) * dt; // More air control
  if (phys->isSliding)
    accel = 0.0f; // No acceleration while sliding

  float dvx = 0, dvy = 0;
  bool isMoving = false;

  if (Input::IsKeyDown(SDL_SCANCODE_W)) {
    dvx += cos(t->rot) * accel;
    dvy += sin(t->rot) * accel;
    isMoving = true;
  }
  if (Input::IsKeyDown(SDL_SCANCODE_S)) {
    dvx -= cos(t->rot) * accel;
    dvy -= sin(t->rot) * accel;
    isMoving = true;
  }
  if (Input::IsKeyDown(SDL_SCANCODE_A)) {
    dvx += cos(t->rot - M_PI / 2) * accel;
    dvy += sin(t->rot - M_PI / 2) * accel;
    isMoving = true;
  }
  if (Input::IsKeyDown(SDL_SCANCODE_D)) {
    dvx += cos(t->rot + M_PI / 2) * accel;
    dvy += sin(t->rot + M_PI / 2) * accel;
    isMoving = true;
  }

  phys->velX += dvx;
  phys->velY += dvy;

  // Crouch & Slide
  bool wantsCrouch =
      Input::IsKeyDown(SDL_SCANCODE_LCTRL) || Input::IsKeyDown(SDL_SCANCODE_C);
  if (wantsCrouch && phys->isGrounded && !phys->isSliding) {
    float speed = sqrt(phys->velX * phys->velX + phys->velY * phys->velY);
    if (speed > 4.0f) {
      phys->isSliding = true;
      phys->slideTimer = 1.0f;
    }
  }
  if (!wantsCrouch)
    phys->isSliding = false;

  // Limit speed (higher limit while sliding or grappling)
  float maxSpeed = phys->isSliding ? p->speed * 2.0f : p->speed;
  float currentSpeed = sqrt(phys->velX * phys->velX + phys->velY * phys->velY);
  if (currentSpeed > maxSpeed && !m_IsGrappling && !phys->isSliding) {
    phys->velX = (phys->velX / currentSpeed) * maxSpeed;
    phys->velY = (phys->velY / currentSpeed) * maxSpeed;
  }

  // Jump & Double Jump
  if (Input::IsKeyPressed(SDL_SCANCODE_SPACE)) {
    bool canJump = phys->isGrounded || phys->isWallRunning || m_IsGrappling;
    bool canDouble = !canJump && phys->doubleJumpCount < phys->maxDoubleJumps;

    if (canJump || canDouble) {
      phys->velZ = p->jumpForce;
      if (canDouble) {
        phys->doubleJumpCount++;
        // Visual/Audio kick for double jump
        m_ShakeTimer = 0.1f;
        m_ShakeIntensity = 0.05f;
      }
      phys->isGrounded = false;
      phys->isWallRunning = false;
      phys->isSliding = false;
      m_IsGrappling = false;
      if (m_SfxJump)
        Mix_PlayChannel(-1, m_SfxJump, 0);
    }
  }

  // Shooting
  bool wantsGrapple = Input::IsMouseButtonPressed(SDL_BUTTON_RIGHT) ||
                      (Input::IsKeyDown(SDL_SCANCODE_E) &&
                       Input::IsMouseButtonPressed(SDL_BUTTON_LEFT));

  if (Input::IsMouseButtonDown(SDL_BUTTON_LEFT) &&
      !Input::IsKeyDown(SDL_SCANCODE_E)) {
    if (!weapon->isDrawing && weapon->cooldown <= 0) {
      weapon->isDrawing = true;
      weapon->drawTime = 0.0f;
    } else if (weapon->isDrawing) {
      weapon->drawTime += dt;
      if (weapon->drawTime > 1.0f)
        weapon->drawTime = 1.0f;
    }
  } else if (wantsGrapple && weapon->cooldown <= 0) {
    // Grapple!
    if (m_SfxGrapple)
      Mix_PlayChannel(-1, m_SfxGrapple, 0);
    auto arrow = m_Registry.CreateEntity();
    float ax = t->x + cos(t->rot) * 0.5f;
    float ay = t->y + sin(t->rot) * 0.5f;
    m_Registry.AddComponent<Transform3DComponent>(
        arrow, {ax, ay, t->z, t->rot, t->pitch});
    m_Registry.AddComponent<ProjectileComponent>(
        arrow, {ProjectileComponent::Grapple, 0.0f, true, 2.0f});
    float speed = 25.0f;
    m_Registry.AddComponent<PhysicsComponent>(
        arrow, {cos(t->rot) * speed, sin(t->rot) * speed, t->pitch * 0.05f,
                0.0f, false, false, 0.0f, 0.0f});
    weapon->cooldown = 1.0f;
  } else {
    if (weapon->isDrawing) {
      // Fire Arrow
      float power = std::min(1.0f, weapon->drawTime); // Max power at 1 second
      weapon->isDrawing = false;
      weapon->cooldown = 0.5f;
      if (m_SfxShoot)
        Mix_PlayChannel(-1, m_SfxShoot, 0);

      // Recoil based on power
      t->pitch += 10.0f + (power * 20.0f);
      m_ShakeTimer = 0.1f + (power * 0.1f);
      m_ShakeIntensity = 0.04f + (power * 0.06f);

      auto arrow = m_Registry.CreateEntity();
      float ax = t->x + cos(t->rot) * 0.5f;
      float ay = t->y + sin(t->rot) * 0.5f;

      m_Registry.AddComponent<Transform3DComponent>(
          arrow, {ax, ay, t->z, t->rot, t->pitch});
      m_Registry.AddComponent<ProjectileComponent>(
          arrow, {ProjectileComponent::Arrow, 50.0f, true, 5.0f});

      // Toned down speed (Min 8, Max 25)
      float speed = 8.0f + (power * 17.0f);

      // Vertical Bias:
      // At low power, the arrow has a downward bias relative to aim.
      // At high power, it has an upward boost.
      float verticalBias = -1.5f + (power * 3.5f);
      float vz = (t->pitch * 0.03f) + verticalBias;

      m_Registry.AddComponent<PhysicsComponent>(
          arrow, {cos(t->rot) * speed, sin(t->rot) * speed, vz, 15.0f, false,
                  false, 0.0f, 0.0f});
      m_Registry.AddComponent<BillboardComponent>(
          arrow, {m_BowIdle, 0.2f, 0.2f, 0.2f, true});
    }
  }

  if (weapon->cooldown > 0)
    weapon->cooldown -= dt;
}

void JumpShootGame::HandleInputMenu() {
  int w = m_Width;
  int h = m_Height;
  int mx, my;
  Input::GetMousePosition(mx, my);

  int btnW = 240;
  int btnH = 50;
  int startY = 300;
  int gap = 70;
  int startX = w / 2 - btnW / 2;

  bool action = false;

  int numButtons = m_InOptions ? 2 : 3;

  // Mouse Interaction
  for (int i = 0; i < numButtons; i++) {
    int by = startY + gap * i;
    if (mx >= startX && mx <= startX + btnW && my >= by && my <= by + btnH) {
      if (m_MenuSelection != i)
        m_MenuSelection = i;

      if (Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
        action = true;
      }
    }
  }

  // Keyboard Interaction
  if (Input::IsKeyPressed(SDL_SCANCODE_W) ||
      Input::IsKeyPressed(SDL_SCANCODE_UP)) {
    m_MenuSelection--;
    if (m_MenuSelection < 0)
      m_MenuSelection = numButtons - 1;
  }
  if (Input::IsKeyPressed(SDL_SCANCODE_S) ||
      Input::IsKeyPressed(SDL_SCANCODE_DOWN)) {
    m_MenuSelection++;
    if (m_MenuSelection > numButtons - 1)
      m_MenuSelection = 0;
  }

  if (Input::IsKeyPressed(SDL_SCANCODE_RETURN) ||
      Input::IsKeyPressed(SDL_SCANCODE_SPACE)) {
    action = true;
  }

  if (Input::IsKeyPressed(Config::GetKeybind(GameAction::ToggleFullScreen))) {
    ToggleFullScreen();
  }

  if (action) {
    if (!m_InOptions) {
      if (m_MenuSelection == 0) { // Play
        InitGame();
        m_State = GameState::Playing;
        SDL_SetRelativeMouseMode(SDL_TRUE);
      } else if (m_MenuSelection == 1) { // Options
        m_InOptions = true;
        m_MenuSelection = 0;
      } else if (m_MenuSelection == 2) { // Quit
        m_IsRunning = false;
      }
    } else {
      if (m_MenuSelection == 0) { // Toggle FS
        ToggleFullScreen();
      } else if (m_MenuSelection == 1) { // Back
        m_InOptions = false;
        m_MenuSelection = 1;
      }
    }
  }
}

void JumpShootGame::HandleInputPause() {
  if (Input::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
    if (m_InOptions) {
      m_InOptions = false;
      m_MenuSelection = 1;
    } else {
      m_State = GameState::Playing;
      SDL_SetRelativeMouseMode(SDL_TRUE);
    }
    return;
  }

  int w = m_Width;
  int h = m_Height;
  int mx, my;
  Input::GetMousePosition(mx, my);

  int btnW = 240;
  int btnH = 50;
  int startY = 250;
  int gap = 70;
  int startX = w / 2 - btnW / 2;

  bool action = false;

  int numButtons = m_InOptions ? 2 : 4;

  // Mouse Interaction
  for (int i = 0; i < numButtons; i++) {
    int by = startY + gap * i;
    if (mx >= startX && mx <= startX + btnW && my >= by && my <= by + btnH) {
      if (m_MenuSelection != i)
        m_MenuSelection = i;

      if (Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
        action = true;
      }
    }
  }

  // Keyboard Interaction
  if (Input::IsKeyPressed(SDL_SCANCODE_W) ||
      Input::IsKeyPressed(SDL_SCANCODE_UP)) {
    m_MenuSelection--;
    if (m_MenuSelection < 0)
      m_MenuSelection = numButtons - 1;
  }
  if (Input::IsKeyPressed(SDL_SCANCODE_S) ||
      Input::IsKeyPressed(SDL_SCANCODE_DOWN)) {
    m_MenuSelection++;
    if (m_MenuSelection > numButtons - 1)
      m_MenuSelection = 0;
  }

  if (Input::IsKeyPressed(SDL_SCANCODE_RETURN) ||
      Input::IsKeyPressed(SDL_SCANCODE_SPACE)) {
    action = true;
  }

  if (Input::IsKeyPressed(Config::GetKeybind(GameAction::ToggleFullScreen))) {
    ToggleFullScreen();
  }

  if (action) {
    if (!m_InOptions) {
      if (m_MenuSelection == 0) { // Resume
        m_State = GameState::Playing;
        SDL_SetRelativeMouseMode(SDL_TRUE);
      } else if (m_MenuSelection == 1) { // Options
        m_InOptions = true;
        m_MenuSelection = 0;
      } else if (m_MenuSelection == 2) { // Main Menu
        m_State = GameState::MainMenu;
        m_MenuSelection = 0;
      } else if (m_MenuSelection == 3) { // Quit
        m_IsRunning = false;
      }
    } else {
      if (m_MenuSelection == 0) { // Toggle FS
        ToggleFullScreen();
      } else if (m_MenuSelection == 1) { // Back
        m_InOptions = false;
        m_MenuSelection = 1;
      }
    }
  }
}
