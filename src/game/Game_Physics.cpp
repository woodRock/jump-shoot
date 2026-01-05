#include "../engine/Components.h"
#include "../engine/TextureManager.h"
#include "JumpShootGame.h"
#include <cmath>

using namespace PixelsEngine;

void JumpShootGame::UpdatePhysics(float dt) {

  if (m_State != GameState::Playing)
    return;

  auto *phys = m_Registry.GetComponent<PhysicsComponent>(m_PlayerEntity);

  auto *t = m_Registry.GetComponent<Transform3DComponent>(m_PlayerEntity);

  if (phys && t) {

        if (phys->isGrounded) {

    

          phys->wallRunTimer = 5.0f; // Reset stamina on ground

    

          phys->doubleJumpCount = 0; // Reset double jump

        }

    

        // Sliding height and friction

    

        float eyeHeight = phys->isSliding ? 0.25f : 0.5f;

    

        float currentFriction =

            phys->isGrounded ? phys->friction : phys->friction * 0.2f;

    

        if (phys->isSliding)

          currentFriction *= 0.1f; // Much less friction while sliding

    

        if (m_IsGrappling) {

            float dx = m_GrapplePoint.x - t->x;

            float dy = m_GrapplePoint.y - t->y;

            float dz = m_GrapplePoint.z - t->z;

            float dist = sqrt(dx*dx + dy*dy + dz*dz);

            

            if (dist < 1.0f) {

                m_IsGrappling = false;

                // Boost up slightly on arrival

                phys->velZ = 5.0f;

            } else {

                float speed = 20.0f;

                phys->velX = (dx / dist) * speed;

                phys->velY = (dy / dist) * speed;

                phys->velZ = (dz / dist) * speed;

            }

        }

    

        // Wall Run Logic

        if (!phys->isGrounded && phys->wallRunTimer > 0.0f) {

            float checkDist = 1.0f;

            // Check Left (-90 deg) and Right (+90 deg) relative to player rotation

            // Note: rot is in radians. 

            float leftX = t->x + cos(t->rot - M_PI/2.0f) * checkDist;

            float leftY = t->y + sin(t->rot - M_PI/2.0f) * checkDist;

            float rightX = t->x + cos(t->rot + M_PI/2.0f) * checkDist;

            float rightY = t->y + sin(t->rot + M_PI/2.0f) * checkDist;

    

            // Assuming tile 2 is the only runnable wall (Mossy)

            bool wallLeft = (m_Map.Get((int)leftX, (int)leftY) == 2);

            bool wallRight = (m_Map.Get((int)rightX, (int)rightY) == 2);

            

            if (wallLeft || wallRight) {

                 // Must have some speed (momentum)

                 float speed = sqrt(phys->velX * phys->velX + phys->velY * phys->velY);

                 if (speed > 0.5f) {

                     phys->isWallRunning = true;

                     phys->wallRunTimer -= dt;

                     phys->velZ = 0.0f; // No gravity while running

                     phys->doubleJumpCount = 0; // Allow jump off wall

                 } else {

                     phys->isWallRunning = false;

                 }

            } else {

                phys->isWallRunning = false;

            }

        } else {

            phys->isWallRunning = false;

        }

    if (!phys->isGrounded && !phys->isWallRunning) {

      phys->velZ -= phys->gravity * dt;
    }

    t->z += phys->velZ * dt;

    // Floor collision & Jump Pads & Lava

    if (t->z < eyeHeight) {
      int tile = m_Map.Get((int)t->x, (int)t->y);

      // Gap (Tile 4) - Fall through
      if (tile == 4) {
          phys->isGrounded = false;
      }
      else {
          if (phys->velZ < -5.0f) {
            m_ShakeTimer = 0.2f;
            m_ShakeIntensity = std::min(0.2f, abs(phys->velZ) * 0.02f);
            // Landing Particles
            for (int i = 0; i < 8; i++) {
              auto p = m_Registry.CreateEntity();
              m_Registry.AddComponent<Transform3DComponent>(
                  p, {t->x, t->y, eyeHeight, 0, 0});
              m_Registry.AddComponent<ParticleComponent>(
                  p, {
                         ((rand() % 100) / 50.0f - 1.0f) * 2.0f,
                         ((rand() % 100) / 50.0f - 1.0f) * 2.0f,
                         ((rand() % 100) / 100.0f) * 2.0f,
                         0.3f,
                         0.5f,
                         {200, 200, 200, 255},
                         1.5f
                     });
            }
          }

          if (tile == 3) { // Jump Pad
            phys->velZ = 12.0f;
            phys->isGrounded = false;
            if (m_SfxJump)
              Mix_PlayChannel(-1, m_SfxJump, 0);
          } else {
            t->z = eyeHeight;
            phys->velZ = 0;
            if (!phys->isGrounded) {
              if (m_SfxJump)
                Mix_PlayChannel(-1, m_SfxJump, 0);
            }
            phys->isGrounded = true;
            // Save Checkpoint if on normal floor
            if (tile == 0) {
              auto *ctrl =
                  m_Registry.GetComponent<PlayerControlComponent>(m_PlayerEntity);
              ctrl->spawnX = t->x;
              ctrl->spawnY = t->y;
              ctrl->spawnZ = t->z;
            }
          }
      }

    } else if (t->z > eyeHeight) {

      phys->isGrounded = false;
    }

    // Kill Plane Check
    if (t->z < -20.0f) {
        auto *ctrl = m_Registry.GetComponent<PlayerControlComponent>(m_PlayerEntity);
        t->x = ctrl->spawnX;
        t->y = ctrl->spawnY;
        t->z = ctrl->spawnZ;
        phys->velX = 0;
        phys->velY = 0;
        phys->velZ = 0;
        phys->isGrounded = true; 
        m_IsGrappling = false;
        if (m_SfxHit) Mix_PlayChannel(-1, m_SfxHit, 0);
    }

    // Horizontal velocity (Axis-Separate for sliding)

    // X Axis
    t->x += phys->velX * dt;
    int tileX = m_Map.Get((int)t->x, (int)t->y);
    if (tileX == 1 || tileX == 2) {
        t->x -= phys->velX * dt;
        if (tileX == 2 && !phys->isGrounded) {
             phys->velX = 0; // Slide along wall
        } else {
             phys->velX *= -0.5f;
        }
    }

    // Y Axis
    t->y += phys->velY * dt;
    int tileY = m_Map.Get((int)t->x, (int)t->y);
    if (tileY == 1 || tileY == 2) {
        t->y -= phys->velY * dt;
        if (tileY == 2 && !phys->isGrounded) {
             phys->velY = 0; // Slide along wall
        } else {
             phys->velY *= -0.5f;
        }
    }

    float drag = 1.0f - (currentFriction * dt);
    if (drag < 0) drag = 0;
    phys->velX *= drag;
    phys->velY *= drag;
  }
}

void JumpShootGame::UpdateProjectiles(float dt) {

  if (m_State != GameState::Playing)
    return;

  auto &projectiles = m_Registry.View<ProjectileComponent>();

  std::vector<Entity> toDestroy;

  for (auto &pair : projectiles) {

    Entity entity = pair.first;

    auto *p = &pair.second;

    if (!m_Registry.HasComponent<PhysicsComponent>(entity) ||
        !m_Registry.HasComponent<Transform3DComponent>(entity))
      continue;

    auto *phys = m_Registry.GetComponent<PhysicsComponent>(entity);

    auto *t = m_Registry.GetComponent<Transform3DComponent>(entity);

    t->x += phys->velX * dt;

    t->y += phys->velY * dt;

    t->z += phys->velZ * dt;

    phys->velZ -= 12.0f * dt; // Increased from 5.0

    p->lifeTime -= dt;

    bool hitWall = m_Map.Get(int(t->x), int(t->y)) > 0;
    bool hitFloor = t->z < 0;

    // Target Collision (Check BEFORE wall collision so we can hit targets on walls/pillars)
    auto &targets = m_Registry.View<TargetComponent>();
    bool hitTarget = false;
    for (auto &tPair : targets) {
      Entity targetEnt = tPair.first;
      auto *tcomp = &tPair.second;

      if (!m_Registry.HasComponent<Transform3DComponent>(targetEnt) ||
          !m_Registry.HasComponent<ColliderComponent>(targetEnt))
        continue;

      auto *tt = m_Registry.GetComponent<Transform3DComponent>(targetEnt);
      auto *tc = m_Registry.GetComponent<ColliderComponent>(targetEnt);

      if (tcomp->isDestroyed)
        continue;

      float dist = sqrt(pow(t->x - tt->x, 2) + pow(t->y - tt->y, 2));
      if (dist < tc->radius && t->z < tt->z + 0.5f && t->z > tt->z - 0.5f) {
        // Hit
        tcomp->isDestroyed = true;
        m_TargetsDestroyed++;
        m_HitmarkerTimer = 0.15f;

        PlaySpatialSfx(m_SfxHit, tt->x, tt->y, tt->z);

        m_ShakeTimer = 0.3f;
        m_ShakeIntensity = 0.1f;

        auto *bill = m_Registry.GetComponent<BillboardComponent>(targetEnt);
        if (bill)
          bill->texture = TextureManager::LoadTexture(m_Renderer,
                                                      "assets/target_broken.png");

        // Target explosion particles
        for (int i = 0; i < 15; i++) {
          auto frag = m_Registry.CreateEntity();
          m_Registry.AddComponent<Transform3DComponent>(
              frag, {tt->x, tt->y, tt->z + 0.2f, 0, 0});
          m_Registry.AddComponent<ParticleComponent>(
              frag, {((rand() % 100) / 50.0f - 1.0f) * 5.0f,
                     ((rand() % 100) / 50.0f - 1.0f) * 5.0f,
                     ((rand() % 100) / 50.0f) * 4.0f, 1.0f, 1.0f,
                     {255, 0, 0, 255}, 3.0f});
        }

        hitTarget = true;
        break;
      }
    }

    if (hitTarget) {
        toDestroy.push_back(entity);
        continue;
    }

    if (p->lifeTime <= 0 || hitFloor || hitWall) {
      if (hitWall || hitFloor) {
        PlaySpatialSfx(m_SfxHit, t->x, t->y, t->z);
        // Spawn fragments
        for (int i = 0; i < 5; i++) {
          auto frag = m_Registry.CreateEntity();
          m_Registry.AddComponent<Transform3DComponent>(
              frag, {t->x, t->y, t->z, 0, 0});
          m_Registry.AddComponent<ParticleComponent>(
              frag, {((rand() % 100) / 50.0f - 1.0f) * 2.0f,
                     ((rand() % 100) / 50.0f - 1.0f) * 2.0f,
                     ((rand() % 100) / 100.0f) * 5.0f,
                     0.5f + (rand() % 100) / 100.0f, 1.0f,
                     {200, 150, 100, 255}, 2.0f});
        }
        if (p->type == ProjectileComponent::Grapple) {
          m_IsGrappling = true;
          m_GrapplePoint = {t->x, t->y, t->z};
          m_ShakeTimer = 0.15f;
          m_ShakeIntensity = 0.05f;
        }
      }
      toDestroy.push_back(entity);
      continue;
    }
  }

  for (auto e : toDestroy) {

    m_Registry.DestroyEntity(e);
  }
}
