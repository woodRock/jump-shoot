#pragma once
#include "Texture.h"
#include <SDL2/SDL.h>
#include <memory>
#include <string>

namespace PixelsEngine {

struct Transform3DComponent {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;   // 0 is floor
  float rot = 0.0f; // Yaw
  float pitch = 0.0f;
};

struct BillboardComponent {
  std::shared_ptr<Texture> texture;
  float scale = 1.0f;
  float width = 0.5f; // World units
  float height = 0.5f;
  bool alwaysFaceCamera = true;
};

struct PhysicsComponent {
  float velX = 0.0f;
  float velY = 0.0f;
  float velZ = 0.0f;
  float gravity = 15.0f;
  bool isGrounded = false;
  bool isWallRunning = false;
  float wallRunTimer = 0.0f;
  float friction = 5.0f;

  // New Movement Tech
  bool isSliding = false;
  float slideTimer = 0.0f;
  int doubleJumpCount = 0;
  int maxDoubleJumps = 1;
};

struct PlayerControlComponent {
  float speed = 5.0f;
  float mouseSensitivity = 0.003f;
  float jumpForce = 6.0f;

  // Checkpoint
  float spawnX = 2.0f;
  float spawnY = 2.0f;
  float spawnZ = 0.5f;
};

struct ColliderComponent {
  float radius = 0.3f;
  float height = 1.0f;
  bool isSolid = true;
};

struct ProjectileComponent {
  enum Type { Arrow, Grapple };
  Type type = Arrow;
  float damage = 10.0f;
  bool active = true;
  float lifeTime = 5.0f;
};

struct TargetComponent {
  bool isDestroyed = false;
  int points = 10;
};

struct ParticleComponent {
  float vx, vy, vz;
  float life;
  float maxLife;
  SDL_Color color;
  float size;
};

struct WeaponComponent {
  float cooldown = 0.0f;
  float drawTime = 0.0f; // How long held
  bool isDrawing = false;
};

} // namespace PixelsEngine
