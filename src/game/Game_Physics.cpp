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
        if (!phys->isGrounded) {
            phys->velZ -= phys->gravity * dt;
        }
        
        t->z += phys->velZ * dt;
        
        // Floor collision
        if (t->z < 0.5f) { // Eye height standing
            t->z = 0.5f;
            phys->velZ = 0;
            phys->isGrounded = true;
        } else if (t->z > 0.5f) {
            phys->isGrounded = false;
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
        phys->velZ -= 5.0f * dt; 
        
        p->lifeTime -= dt;
        if (p->lifeTime <= 0 || t->z < 0 || m_Map.Get(int(t->x), int(t->y)) > 0) {
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
                 auto* bill = m_Registry.GetComponent<BillboardComponent>(targetEnt);
                 if (bill) bill->texture = TextureManager::LoadTexture(m_Renderer, "assets/target_broken.png");
                 
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