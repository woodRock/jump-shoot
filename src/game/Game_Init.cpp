#include "JumpShootGame.h"
#include "../engine/Components.h"
#include "../engine/TextureManager.h"
#include <SDL2/SDL.h>

using namespace PixelsEngine;

JumpShootGame::JumpShootGame() : Application("Jump Shoot - Parkour Archer", 800, 600) {
}

JumpShootGame::~JumpShootGame() {
    if (m_SfxShoot) Mix_FreeChunk(m_SfxShoot);
    if (m_SfxHit) Mix_FreeChunk(m_SfxHit);
    if (m_SfxJump) Mix_FreeChunk(m_SfxJump);
    if (m_SfxGrapple) Mix_FreeChunk(m_SfxGrapple);
    if (m_Ambience) Mix_FreeMusic(m_Ambience);
}

void JumpShootGame::OnStart() {
    m_TextRenderer = std::make_unique<TextRenderer>(m_Renderer, "assets/font.ttf", 24);
    m_Raycaster.Init(m_Renderer);
    
    m_BowIdle = TextureManager::LoadTexture(m_Renderer, "assets/bow_idle.png");
    m_BowDraw = TextureManager::LoadTexture(m_Renderer, "assets/bow_draw.png");
    m_Crosshair = TextureManager::LoadTexture(m_Renderer, "assets/crosshair.png");
    
    m_SfxShoot = Mix_LoadWAV("assets/shoot.wav");
    m_SfxHit = Mix_LoadWAV("assets/hit.wav");
    m_SfxJump = Mix_LoadWAV("assets/jump.wav");
    m_SfxGrapple = Mix_LoadWAV("assets/grapple.wav");
    m_Ambience = Mix_LoadMUS("assets/ambience.mp3"); // Assuming mp3/ogg or wav
    if (!m_Ambience) m_Ambience = Mix_LoadMUS("assets/ambience.wav");
    
    if (m_Ambience) {
        Mix_PlayMusic(m_Ambience, -1);
        Mix_VolumeMusic(32); // Lower volume for ambience
    }
    
    InitGame();
    m_State = GameState::MainMenu;
    SDL_SetRelativeMouseMode(SDL_FALSE);
}

void JumpShootGame::InitGame() {
    m_Registry = Registry();
    m_IsGrappling = false;
    
    // Load Map from file or default if fails
    if (!m_Map.LoadFromFile("assets/level1.map")) {
        // Clear Map fallback
        for (int i=0; i<Map::WIDTH * Map::HEIGHT; i++) m_Map.tiles[i] = 0;
        // Outer Walls
        for (int i=0; i<Map::WIDTH; i++) {
            m_Map.Set(i, 0, 1);
            m_Map.Set(i, Map::HEIGHT-1, 1);
            m_Map.Set(0, i, 1);
            m_Map.Set(Map::WIDTH-1, i, 1);
        }
    }

    // Section 1: Archery Room (Start)
    // Wall separating Room 1 and 2
    for (int y=0; y<10; y++) m_Map.Set(8, y, 1);
    // Opening at y=8
    m_Map.Set(8, 8, 0); m_Map.Set(8, 9, 0);

    // Section 2: Wall Run Corridor
    // Narrow path with a long wall
    for (int x=8; x<20; x++) {
        m_Map.Set(x, 10, 1);
        m_Map.Set(x, 6, 2); // The wall to run on
    }
    
    // Section 3: Grapple Arena
    // Large open space with pillars
    m_Map.Set(15, 15, 1); // Pillar 1
    m_Map.Set(18, 18, 2); // Pillar 2
    m_Map.Set(21, 15, 1); // Pillar 3
    
    // Player Start
    m_PlayerEntity = m_Registry.CreateEntity();
    m_Registry.AddComponent<Transform3DComponent>(m_PlayerEntity, {2.0f, 2.0f, 0.5f, 0.0f, 0.0f});
    m_Registry.AddComponent<PlayerControlComponent>(m_PlayerEntity, {5.0f, 0.003f, 6.0f});
    m_Registry.AddComponent<PhysicsComponent>(m_PlayerEntity, {0,0,0, 15.0f, false, false, 0.0f, 5.0f});
    m_Registry.AddComponent<ColliderComponent>(m_PlayerEntity, {0.3f, 1.0f, true});
    m_Registry.AddComponent<WeaponComponent>(m_PlayerEntity, {0.0f, 0.0f, false});
    
    m_Camera->x = 2.0f; m_Camera->y = 2.0f; m_Camera->z = 0.5f;
    
    auto targetTex = TextureManager::LoadTexture(m_Renderer, "assets/target.png");
    
    // Target 1: Archery Tutorial
    auto t1 = m_Registry.CreateEntity();
    m_Registry.AddComponent<Transform3DComponent>(t1, {6.0f, 4.0f, 0.0f, 0.0f, 0.0f});
    m_Registry.AddComponent<BillboardComponent>(t1, {targetTex, 0.8f, 0.5f, 0.5f, true});
    m_Registry.AddComponent<ColliderComponent>(t1, {0.4f, 1.0f, true});
    m_Registry.AddComponent<TargetComponent>(t1, {false, 10});

    // Target 2: Wall Run End
    auto t2 = m_Registry.CreateEntity();
    m_Registry.AddComponent<Transform3DComponent>(t2, {21.0f, 8.0f, 0.0f, 0.0f, 0.0f});
    m_Registry.AddComponent<BillboardComponent>(t2, {targetTex, 0.8f, 0.5f, 0.5f, true});
    m_Registry.AddComponent<ColliderComponent>(t2, {0.4f, 1.0f, true});
    m_Registry.AddComponent<TargetComponent>(t2, {false, 10});

    // Target 3: Final Challenge (High up or far)
    auto t3 = m_Registry.CreateEntity();
    m_Registry.AddComponent<Transform3DComponent>(t3, {22.0f, 21.0f, 0.0f, 0.0f, 0.0f});
    m_Registry.AddComponent<BillboardComponent>(t3, {targetTex, 1.2f, 0.5f, 0.5f, true});
    m_Registry.AddComponent<ColliderComponent>(t3, {0.6f, 1.0f, true});
    m_Registry.AddComponent<TargetComponent>(t3, {false, 50});
}