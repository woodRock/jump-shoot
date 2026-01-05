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

        

        // Screenshake update

        if (m_ShakeTimer > 0) {

            m_ShakeTimer -= deltaTime; // Real time, not dt

            if (m_ShakeTimer < 0) m_ShakeTimer = 0;

        } else {

            m_ShakeIntensity = 0;

        }



        // Particle System update

        auto& particles = m_Registry.View<ParticleComponent>();

        std::vector<Entity> deadParticles;

        for (auto& pair : particles) {

            Entity e = pair.first;

            auto& p = pair.second;

            auto* t = m_Registry.GetComponent<Transform3DComponent>(e);

            

            if (t) {

                t->x += p.vx * dt;

                t->y += p.vy * dt;

                t->z += p.vz * dt;

                p.vz -= 9.8f * dt; // Gravity

                p.life -= dt;

                if (p.life <= 0) deadParticles.push_back(e);

            }

        }

        for (auto e : deadParticles) m_Registry.DestroyEntity(e);



        // View Bobbing & Sway

        m_SwayTimer += dt * 5.0f;

        if (phys && (abs(phys->velX) > 0.1f || abs(phys->velY) > 0.1f)) {

            m_BobTimer += dt * 10.0f;

        } else {

            m_BobTimer *= 0.9f; // Reset bob

        }



        // Sync Camera to Player + Screenshake

        auto* t = m_Registry.GetComponent<Transform3DComponent>(m_PlayerEntity);

        if (t) {

            m_Camera->x = t->x;

            m_Camera->y = t->y;

            m_Camera->z = t->z;

            

            if (m_ShakeTimer > 0) {

                m_Camera->x += ((rand() % 100) / 50.0f - 1.0f) * m_ShakeIntensity;

                m_Camera->y += ((rand() % 100) / 50.0f - 1.0f) * m_ShakeIntensity;

                m_Camera->z += ((rand() % 100) / 50.0f - 1.0f) * m_ShakeIntensity;

            }



            m_Camera->yaw = t->rot;

            m_Camera->pitch = t->pitch;

        }

    } 

    else if (m_State == GameState::Paused) {

        HandleInputPause();

    }

}



void JumpShootGame::PlaySpatialSfx(Mix_Chunk* chunk, float x, float y, float z) {

    if (!chunk || !m_Camera) return;



    int channel = Mix_PlayChannel(-1, chunk, 0);

    if (channel == -1) return;



    float dx = x - m_Camera->x;

    float dy = y - m_Camera->y;

    float dist = sqrt(dx*dx + dy*dy);

    

    // Volume based on distance

    int volume = (int)(MIX_MAX_VOLUME * (1.0f - std::min(1.0f, dist / 20.0f)));

    Mix_Volume(channel, volume);



    // Panning

    float angle = atan2(dy, dx) - m_Camera->yaw;

    float pan = sin(angle); // -1 left, 1 right

    Uint8 left = (Uint8)(127 * (1.0f - pan));

    Uint8 right = (Uint8)(127 * (1.0f + pan));

    Mix_SetPanning(channel, left, right);

}
