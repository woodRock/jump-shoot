#pragma once
#include "../engine/Application.h"
#include "../engine/Raycaster.h"
#include "../engine/Map.h"
#include "../engine/ECS.h"
#include "../engine/TextRenderer.h"
#include <memory>

enum class GameState {
    MainMenu,
    Playing,
    Paused
};

class JumpShootGame : public PixelsEngine::Application {
public:
    JumpShootGame();
    ~JumpShootGame();

protected:
    void OnStart() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;

private:
    // Modular systems
    void InitGame();
    void InitMainMenu();
    
    void UpdateMainMenu(float dt);
    void UpdateGameplay(float dt);
    
    void RenderMainMenu();
    void RenderGameplay();
    void RenderUI();
    void RenderPauseMenu();

    void HandleInputGameplay(float dt);
    void HandleInputMenu();
    void HandleInputPause();
    
    void UpdatePhysics(float dt);
    void UpdateProjectiles(float dt);
    
    // UI Helpers
    void DrawButton(int x, int y, int w, int h, const std::string& text, bool selected);

    PixelsEngine::Raycaster m_Raycaster;
    PixelsEngine::Map m_Map;
    std::unique_ptr<PixelsEngine::TextRenderer> m_TextRenderer;
    
    std::shared_ptr<PixelsEngine::Texture> m_BowIdle;
    std::shared_ptr<PixelsEngine::Texture> m_BowDraw;
    std::shared_ptr<PixelsEngine::Texture> m_Crosshair;
    
    // Sounds
    Mix_Chunk* m_SfxShoot = nullptr;
    Mix_Chunk* m_SfxHit = nullptr;
    Mix_Chunk* m_SfxJump = nullptr;
    Mix_Chunk* m_SfxGrapple = nullptr;
    Mix_Music* m_Ambience = nullptr;
    
    PixelsEngine::Entity m_PlayerEntity;
    
    GameState m_State = GameState::MainMenu;
    int m_MenuSelection = 0; // 0: Play/Resume, 1: Options, 2: Quit/MainMenu
    
    // Main Menu Camera Props
    float m_MenuCamAngle = 0.0f;
    float m_TimeScale = 1.0f;
    float m_BobTimer = 0.0f;
    float m_SwayTimer = 0.0f;
    
    // Screenshake
    float m_ShakeIntensity = 0.0f;
    float m_ShakeTimer = 0.0f;
    
    bool m_IsGrappling = false;
    struct { float x, y, z; } m_GrapplePoint;

    // Spatial Audio Helper
    void PlaySpatialSfx(Mix_Chunk* chunk, float x, float y, float z);
};
