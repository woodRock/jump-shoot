#include "JumpShootGame.h"
#include "../engine/Components.h"
#include "../engine/TextureManager.h"
#include <SDL2/SDL.h>

using namespace PixelsEngine;

JumpShootGame::JumpShootGame() : Application("Jump Shoot - Parkour Archer", 800, 600) {
}

JumpShootGame::~JumpShootGame() {
}

void JumpShootGame::OnStart() {
    // Init Text Renderer
    m_TextRenderer = std::make_unique<TextRenderer>(m_Renderer, "assets/font.ttf", 24);

    // Init Raycaster
    m_Raycaster.Init(m_Renderer);
    
    // Load UI Assets
    m_BowIdle = TextureManager::LoadTexture(m_Renderer, "assets/bow_idle.png");
    m_BowDraw = TextureManager::LoadTexture(m_Renderer, "assets/bow_draw.png");
    m_Crosshair = TextureManager::LoadTexture(m_Renderer, "assets/crosshair.png");
    
    InitGame();
    
    // Start in Main Menu
    m_State = GameState::MainMenu;
    SDL_SetRelativeMouseMode(SDL_FALSE); // Cursor visible for menu
}

void JumpShootGame::InitGame() {
    m_Registry = Registry(); // Reset Registry
    
    // Init Map
    for (int i=0; i<Map::WIDTH * Map::HEIGHT; i++) m_Map.tiles[i] = 0;
    
    // Walls
    for (int i=0; i<Map::WIDTH; i++) {
        m_Map.Set(i, 0, 1);
        m_Map.Set(i, Map::HEIGHT-1, 1);
        m_Map.Set(0, i, 1);
        m_Map.Set(Map::WIDTH-1, i, 1);
    }
    
    // Pillars
    m_Map.Set(10, 10, 2);
    m_Map.Set(12, 12, 2);
    m_Map.Set(15, 8, 2);
    m_Map.Set(5, 18, 1);
    
    // Setup Player
    m_PlayerEntity = m_Registry.CreateEntity();
    m_Registry.AddComponent<Transform3DComponent>(m_PlayerEntity, {3.5f, 3.5f, 0.5f, 0.0f, 0.0f});
    m_Registry.AddComponent<PlayerControlComponent>(m_PlayerEntity, {5.0f, 0.003f, 6.0f});
    m_Registry.AddComponent<PhysicsComponent>(m_PlayerEntity, {0,0,0, 15.0f, false, 5.0f});
    m_Registry.AddComponent<ColliderComponent>(m_PlayerEntity, {0.3f, 1.0f, true});
    m_Registry.AddComponent<WeaponComponent>(m_PlayerEntity, {0.0f, 0.0f, false});
    
    // Sync Camera
    m_Camera->x = 3.5f;
    m_Camera->y = 3.5f;
    m_Camera->z = 0.5f;
    
    // Spawn Targets
    auto targetTex = TextureManager::LoadTexture(m_Renderer, "assets/target.png");
    
    for (int i=0; i<5; i++) {
        auto e = m_Registry.CreateEntity();
        m_Registry.AddComponent<Transform3DComponent>(e, {10.0f + i*2, 5.0f + (i%2)*5, 0.0f, 0.0f, 0.0f});
        m_Registry.AddComponent<BillboardComponent>(e, {targetTex, 0.8f, 0.5f, 0.5f, true});
        m_Registry.AddComponent<ColliderComponent>(e, {0.4f, 1.0f, true});
        m_Registry.AddComponent<TargetComponent>(e, {false, 10});
    }
}