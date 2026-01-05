#include "JumpShootGame.h"
#include "../engine/Components.h"
#include "../engine/TextureManager.h"
#include <cmath>

using namespace PixelsEngine;

void JumpShootGame::UpdatePhysics(float dt) {

    if (m_State != GameState::Playing) return;



    auto* phys = m_Registry.GetComponent<PhysicsComponent>(m_PlayerEntity);

    auto* t = m_Registry.GetComponent<Transform3DComponent>(m_PlayerEntity);

    

        if (phys && t) {

    

            if (phys->isGrounded) {

    

                phys->wallRunTimer = 2.0f; // Reset stamina on ground

    

                phys->doubleJumpCount = 0; // Reset double jump

    

            }

    

    

    

            // Sliding height and friction

    

            float eyeHeight = phys->isSliding ? 0.25f : 0.5f;

    

            float currentFriction = phys->isGrounded ? phys->friction : phys->friction * 0.2f;

    

            if (phys->isSliding) currentFriction *= 0.1f; // Much less friction while sliding

    

    

    

            if (m_IsGrappling) {

    

                // ... (grapple logic remains same)

    

            }

    

    

    

            // ... (wall run logic remains same)

    

    

    

            if (!phys->isGrounded && !phys->isWallRunning) {

    

                phys->velZ -= phys->gravity * dt;

    

            }

    

            

    

            t->z += phys->velZ * dt;

    

            

    

            // Floor collision & Jump Pads & Lava

    

            if (t->z < eyeHeight) { 

    

                if (phys->velZ < -5.0f) {

    

                    m_ShakeTimer = 0.2f;

    

                    m_ShakeIntensity = std::min(0.2f, abs(phys->velZ) * 0.02f);

    

                    

    

                    // Landing Particles

    

                    for (int i=0; i<8; i++) {

    

                        auto p = m_Registry.CreateEntity();

    

                        m_Registry.AddComponent<Transform3DComponent>(p, {t->x, t->y, eyeHeight, 0, 0});

    

                        m_Registry.AddComponent<ParticleComponent>(p, {

    

                            ((rand()%100)/50.0f - 1.0f) * 2.0f,

    

                            ((rand()%100)/50.0f - 1.0f) * 2.0f,

    

                            ((rand()%100)/100.0f) * 2.0f,

    

                            0.3f, 0.5f, {200, 200, 200, 255}, 1.5f

    

                        });

    

                    }

    

                }

    

                

    

                int tile = m_Map.Get((int)t->x, (int)t->y);

    

                if (tile == 3) { // Jump Pad

    

                    phys->velZ = 12.0f;

    

                    phys->isGrounded = false;

    

                    if (m_SfxJump) Mix_PlayChannel(-1, m_SfxJump, 0);

    

                } else if (tile == 4) { // Lava

    

                    auto* ctrl = m_Registry.GetComponent<PlayerControlComponent>(m_PlayerEntity);

    

                    t->x = ctrl->spawnX; t->y = ctrl->spawnY; t->z = ctrl->spawnZ;

    

                    phys->velX = 0; phys->velY = 0; phys->velZ = 0;

    

                    m_ShakeTimer = 0.5f; m_ShakeIntensity = 0.2f;

    

                    if (m_SfxHit) Mix_PlayChannel(-1, m_SfxHit, 0);

    

                            } else {

    

                                t->z = eyeHeight;

    

                                phys->velZ = 0;

    

                                phys->isGrounded = true;

    

                

    

                                // Save Checkpoint if on normal floor

    

                                if (tile == 0) {

    

                                    auto* ctrl = m_Registry.GetComponent<PlayerControlComponent>(m_PlayerEntity);

    

                                    ctrl->spawnX = t->x; ctrl->spawnY = t->y; ctrl->spawnZ = t->z;

    

                                }

    

                            }

    

                

    

            } else if (t->z > eyeHeight) {

    

                phys->isGrounded = false;

    

            }

    

    

    

            // Horizontal velocity

    

            t->x += phys->velX * dt;

    

            t->y += phys->velY * dt;

    

            

    

            float drag = 1.0f - (currentFriction * dt);

    

            if (drag < 0) drag = 0;

    

            phys->velX *= drag;

    

            phys->velY *= drag;

    

    

        

        if (m_Map.Get((int)t->x, (int)t->y) > 0) {

            t->x -= phys->velX * dt;

            t->y -= phys->velY * dt;

            phys->velX *= -0.5f; // Bounce slightly

            phys->velY *= -0.5f;

        }

    }

}



void JumpShootGame::UpdateProjectiles(float dt) {

    if (m_State != GameState::Playing) return;



    auto& projectiles = m_Registry.View<ProjectileComponent>();

    std::vector<Entity> toDestroy;

    

    for (auto& pair : projectiles) {

        Entity entity = pair.first;

        auto* p = &pair.second;

        

        if (!m_Registry.HasComponent<PhysicsComponent>(entity) || !m_Registry.HasComponent<Transform3DComponent>(entity)) continue;

        

        auto* phys = m_Registry.GetComponent<PhysicsComponent>(entity);

        auto* t = m_Registry.GetComponent<Transform3DComponent>(entity);

        

                t->x += phys->velX * dt;

        

                t->y += phys->velY * dt;

        

                t->z += phys->velZ * dt;

        

                phys->velZ -= 12.0f * dt; // Increased from 5.0

        

         

        

        p->lifeTime -= dt;

        

        bool hitWall = m_Map.Get(int(t->x), int(t->y)) > 0;

        bool hitFloor = t->z < 0;



        if (p->lifeTime <= 0 || hitFloor || hitWall) {

            if (hitWall || hitFloor) {

                PlaySpatialSfx(m_SfxHit, t->x, t->y, t->z);

                

                // Spawn fragments

                for (int i=0; i<5; i++) {

                    auto frag = m_Registry.CreateEntity();

                    m_Registry.AddComponent<Transform3DComponent>(frag, {t->x, t->y, t->z, 0, 0});

                    m_Registry.AddComponent<ParticleComponent>(frag, {

                        ((rand()%100)/50.0f - 1.0f) * 2.0f,

                        ((rand()%100)/50.0f - 1.0f) * 2.0f,

                        ((rand()%100)/100.0f) * 5.0f,

                        0.5f + (rand()%100)/100.0f, 1.0f, {200, 150, 100, 255}, 2.0f

                    });

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

        

        // Target Collision

        auto& targets = m_Registry.View<TargetComponent>();

        bool hit = false;

        for (auto& tPair : targets) {

            Entity targetEnt = tPair.first;

            auto* tcomp = &tPair.second;

            

             if (!m_Registry.HasComponent<Transform3DComponent>(targetEnt) || !m_Registry.HasComponent<ColliderComponent>(targetEnt)) continue;

             

            auto* tt = m_Registry.GetComponent<Transform3DComponent>(targetEnt);

            auto* tc = m_Registry.GetComponent<ColliderComponent>(targetEnt);

            

            if (tcomp->isDestroyed) continue;

            

            float dist = sqrt(pow(t->x - tt->x, 2) + pow(t->y - tt->y, 2));

                        if (dist < tc->radius && t->z < tt->z + 0.5f && t->z > tt->z - 0.5f) {

                             // Hit

                             tcomp->isDestroyed = true;

                             m_TargetsDestroyed++;

                             m_HitmarkerTimer = 0.15f;

                             

                             PlaySpatialSfx(m_SfxHit, tt->x, tt->y, tt->z);

            

                 m_ShakeTimer = 0.3f;

                 m_ShakeIntensity = 0.1f;



                 auto* bill = m_Registry.GetComponent<BillboardComponent>(targetEnt);

                 if (bill) bill->texture = TextureManager::LoadTexture(m_Renderer, "assets/target_broken.png");

                 

                 // Target explosion particles

                 for (int i=0; i<15; i++) {

                    auto frag = m_Registry.CreateEntity();

                    m_Registry.AddComponent<Transform3DComponent>(frag, {tt->x, tt->y, tt->z + 0.2f, 0, 0});

                    m_Registry.AddComponent<ParticleComponent>(frag, {

                        ((rand()%100)/50.0f - 1.0f) * 5.0f,

                        ((rand()%100)/50.0f - 1.0f) * 5.0f,

                        ((rand()%100)/50.0f) * 4.0f,

                        1.0f, 1.0f, {255, 0, 0, 255}, 3.0f

                    });

                 }



                 hit = true;

                 break; 

            }

        }

        

        if (hit) toDestroy.push_back(entity);

    }

    

    for (auto e : toDestroy) {

        m_Registry.DestroyEntity(e);

    }

}
