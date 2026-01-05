#include "JumpShootGame.h"
#include <SDL2/SDL.h>
#include "../engine/Components.h"

using namespace PixelsEngine;

void JumpShootGame::DrawButton(int x, int y, int w, int h, const std::string& text, bool selected) {
    SDL_Rect rect = {x, y, w, h};
    if (selected) SDL_SetRenderDrawColor(m_Renderer, 200, 50, 50, 200);
    else SDL_SetRenderDrawColor(m_Renderer, 50, 50, 50, 200);
    SDL_RenderFillRect(m_Renderer, &rect);
    SDL_SetRenderDrawColor(m_Renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(m_Renderer, &rect);
    m_TextRenderer->RenderTextCentered(text, x + w/2, y + h/2 - 12, {255, 255, 255, 255});
}

void JumpShootGame::RenderMainMenu() {
    int w = m_Width; int h = m_Height;
    m_TextRenderer->RenderTextCentered("JUMP SHOOT", w/2, 100, {255, 200, 50, 255});
    m_TextRenderer->RenderTextCentered("Parkour Archer", w/2, 140, {200, 200, 200, 255});
    int btnW = 240; int btnH = 50; int startY = 300; int gap = 70;
    if (!m_InOptions) {
        DrawButton(w/2 - btnW/2, startY, btnW, btnH, "PLAY", m_MenuSelection == 0);
        DrawButton(w/2 - btnW/2, startY + gap, btnW, btnH, "OPTIONS", m_MenuSelection == 1);
        DrawButton(w/2 - btnW/2, startY + gap*2, btnW, btnH, "QUIT", m_MenuSelection == 2);
    } else {
        m_TextRenderer->RenderTextCentered("OPTIONS", w/2, 240, {255, 255, 255, 255});
        bool isFullscreen = SDL_GetWindowFlags(m_Window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
        std::string fsText = isFullscreen ? "FULLSCREEN: ON" : "FULLSCREEN: OFF";
        DrawButton(w/2 - btnW/2, startY, btnW, btnH, fsText, m_MenuSelection == 0);
        DrawButton(w/2 - btnW/2, startY + gap, btnW, btnH, "BACK", m_MenuSelection == 1);
    }
}

void JumpShootGame::RenderPauseMenu() {
    int w = m_Width; int h = m_Height;
    SDL_SetRenderDrawColor(m_Renderer, 0, 0, 0, 150);
    SDL_Rect rect = {0, 0, w, h}; SDL_RenderFillRect(m_Renderer, &rect);
    m_TextRenderer->RenderTextCentered("PAUSED", w/2, 100, {255, 255, 255, 255});
    int btnW = 240; int btnH = 50; int startY = 250; int gap = 70;
    if (!m_InOptions) {
        DrawButton(w/2 - btnW/2, startY, btnW, btnH, "RESUME", m_MenuSelection == 0);
        DrawButton(w/2 - btnW/2, startY + gap, btnW, btnH, "OPTIONS", m_MenuSelection == 1);
        DrawButton(w/2 - btnW/2, startY + gap*2, btnW, btnH, "MAIN MENU", m_MenuSelection == 2);
        DrawButton(w/2 - btnW/2, startY + gap*3, btnW, btnH, "QUIT", m_MenuSelection == 3);
    } else {
        m_TextRenderer->RenderTextCentered("OPTIONS", w/2, 180, {255, 255, 255, 255});
        bool isFullscreen = SDL_GetWindowFlags(m_Window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
        std::string fsText = isFullscreen ? "FULLSCREEN: ON" : "FULLSCREEN: OFF";
        DrawButton(w/2 - btnW/2, startY, btnW, btnH, fsText, m_MenuSelection == 0);
        DrawButton(w/2 - btnW/2, startY + gap, btnW, btnH, "BACK", m_MenuSelection == 1);
    }
}

void JumpShootGame::RenderUI() {
    int w = m_Width; int h = m_Height;
    auto* t = m_Registry.GetComponent<Transform3DComponent>(m_PlayerEntity);
    if (t) {
        std::string tutorial = "";
        if (t->x < 8) tutorial = "SECTION 1: ARCHERY. Hold Left Click to draw, release to fire.";
        else if (t->x >= 8 && t->x < 20 && t->y < 12) tutorial = "SECTION 2: PARKOUR. Jump against the mossy wall to Wall Run!";
        else if (t->x > 12 && t->y >= 12) tutorial = "SECTION 3: GRAPPLE. Right Click (or E+Click) a pillar to Zip!";
        if (!tutorial.empty()) m_TextRenderer->RenderTextWrappedCentered(tutorial, w/2, 50, 600, {255, 255, 255, 200});
    }

    m_Crosshair->Render(w/2 - 16, h/2 - 16);
    
    auto* weapon = m_Registry.GetComponent<WeaponComponent>(m_PlayerEntity);
    auto* phys = m_Registry.GetComponent<PhysicsComponent>(m_PlayerEntity);

    if (weapon) {
        Texture* bowTex = weapon->isDrawing ? m_BowDraw.get() : m_BowIdle.get();
        int bowW = 400; int bowH = 400;
        float swayX = sin(m_SwayTimer * 0.5f) * 15.0f;
        float swayY = cos(m_SwayTimer) * 10.0f;
        int bowX = w/2 - bowW/2 + (int)swayX;
        int bowY = h - bowH + 50 + (int)(sin(m_BobTimer) * 15) + (int)swayY;
        if (weapon->isDrawing) { bowX += (rand() % 4); bowY += 20 + (rand() % 4); }
        if (m_ShakeTimer > 0) {
            bowX += (int)(((rand() % 100) / 50.0f - 1.0f) * m_ShakeIntensity * 100.0f);
            bowY += (int)(((rand() % 100) / 50.0f - 1.0f) * m_ShakeIntensity * 100.0f);
        }
        bowTex->Render(bowX, bowY, bowW, bowH);
    }

    if (m_HitmarkerTimer > 0) {
        SDL_SetRenderDrawColor(m_Renderer, 255, 255, 255, 255);
        int size = 10;
        SDL_RenderDrawLine(m_Renderer, w/2 - size, h/2 - size, w/2 - size/2, h/2 - size/2);
        SDL_RenderDrawLine(m_Renderer, w/2 + size, h/2 - size, w/2 + size/2, h/2 - size/2);
        SDL_RenderDrawLine(m_Renderer, w/2 - size, h/2 + size, w/2 - size/2, h/2 + size/2);
        SDL_RenderDrawLine(m_Renderer, w/2 + size, h/2 + size, w/2 + size/2, h/2 + size/2);
    }

    if (phys) {
        float speed = sqrt(phys->velX*phys->velX + phys->velY*phys->velY);
        std::string speedStr = "SPD: " + std::to_string((int)(speed * 10)) + " UPS";
        m_TextRenderer->RenderText(speedStr, 20, h - 40, {255, 255, 255, 255});
        if (!phys->isGrounded) {
            SDL_Rect barBg = {20, h - 60, 100, 10}; SDL_SetRenderDrawColor(m_Renderer, 50, 50, 50, 200); SDL_RenderFillRect(m_Renderer, &barBg);
            SDL_Rect barFg = {20, h - 60, (int)(phys->wallRunTimer * 50), 10}; SDL_SetRenderDrawColor(m_Renderer, 50, 200, 255, 255); SDL_RenderFillRect(m_Renderer, &barFg);
        }
    }

    char timerBuf[32]; snprintf(timerBuf, 32, "TIME: %.2fs", m_RunTimer);
    m_TextRenderer->RenderText(timerBuf, w - 180, 20, {255, 255, 255, 255});
    std::string targetStr = "TARGETS: " + std::to_string(m_TargetsDestroyed) + "/" + std::to_string(m_TotalTargets);
    m_TextRenderer->RenderText(targetStr, w - 210, 50, {255, 255, 255, 255});

    if (m_GameFinished) {
        SDL_SetRenderDrawColor(m_Renderer, 0, 0, 0, 180); SDL_Rect dim = {0, 0, w, h}; SDL_RenderFillRect(m_Renderer, &dim);
        m_TextRenderer->RenderTextCentered("COURSE COMPLETE!", w/2, h/2 - 40, {255, 200, 50, 255});
        char finalTime[64]; snprintf(finalTime, 64, "FINAL TIME: %.3fs", m_RunTimer);
        m_TextRenderer->RenderTextCentered(finalTime, w/2, h/2 + 10, {255, 255, 255, 255});
        m_TextRenderer->RenderTextCentered("Press SPACE to restart", w/2, h/2 + 60, {200, 200, 200, 255});
    }
}