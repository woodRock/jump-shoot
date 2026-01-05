#include "JumpShootGame.h"
#include <SDL2/SDL.h>
#include "../engine/Components.h"

using namespace PixelsEngine;

void JumpShootGame::DrawButton(int x, int y, int w, int h, const std::string& text, bool selected) {
    SDL_Rect rect = {x, y, w, h};
    if (selected) {
        SDL_SetRenderDrawColor(m_Renderer, 200, 50, 50, 200);
    } else {
        SDL_SetRenderDrawColor(m_Renderer, 50, 50, 50, 200);
    }
    SDL_RenderFillRect(m_Renderer, &rect);
    
    SDL_SetRenderDrawColor(m_Renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(m_Renderer, &rect);
    
    m_TextRenderer->RenderTextCentered(text, x + w/2, y + h/2 - 12, {255, 255, 255, 255});
}

void JumpShootGame::RenderMainMenu() {
    int w, h;
    SDL_GetRendererOutputSize(m_Renderer, &w, &h);
    
    // Title
    m_TextRenderer->RenderTextCentered("JUMP SHOOT", w/2, 100, {255, 200, 50, 255});
    m_TextRenderer->RenderTextCentered("Parkour Archer", w/2, 140, {200, 200, 200, 255});
    
    int btnW = 200;
    int btnH = 50;
    int startY = 300;
    int gap = 70;
    
    DrawButton(w/2 - btnW/2, startY, btnW, btnH, "PLAY", m_MenuSelection == 0);
    DrawButton(w/2 - btnW/2, startY + gap, btnW, btnH, "OPTIONS", m_MenuSelection == 1);
    DrawButton(w/2 - btnW/2, startY + gap*2, btnW, btnH, "QUIT", m_MenuSelection == 2);
}

void JumpShootGame::RenderPauseMenu() {
    int w, h;
    SDL_GetRendererOutputSize(m_Renderer, &w, &h);
    
    // Darken background
    SDL_SetRenderDrawColor(m_Renderer, 0, 0, 0, 150);
    SDL_Rect rect = {0, 0, w, h};
    SDL_RenderFillRect(m_Renderer, &rect);
    
    m_TextRenderer->RenderTextCentered("PAUSED", w/2, 100, {255, 255, 255, 255});
    
    int btnW = 200;
    int btnH = 50;
    int startY = 250;
    int gap = 70;
    
    DrawButton(w/2 - btnW/2, startY, btnW, btnH, "RESUME", m_MenuSelection == 0);
    DrawButton(w/2 - btnW/2, startY + gap, btnW, btnH, "SAVE GAME", m_MenuSelection == 1);
    DrawButton(w/2 - btnW/2, startY + gap*2, btnW, btnH, "MAIN MENU", m_MenuSelection == 2);
    DrawButton(w/2 - btnW/2, startY + gap*3, btnW, btnH, "QUIT", m_MenuSelection == 3);
}

void JumpShootGame::RenderUI() {

    int w, h;

    SDL_GetRendererOutputSize(m_Renderer, &w, &h);

    

    // Tutorial Text

    auto* t = m_Registry.GetComponent<Transform3DComponent>(m_PlayerEntity);

    if (t) {

        std::string tutorial = "";

        if (t->x < 8) {

            tutorial = "SECTION 1: ARCHERY. Hold Left Click to draw, release to fire.";

        } else if (t->x >= 8 && t->x < 20 && t->y < 12) {

            tutorial = "SECTION 2: PARKOUR. Jump against the mossy wall to Wall Run!";

        } else if (t->x > 12 && t->y >= 12) {

            tutorial = "SECTION 3: GRAPPLE. Right Click (or E+Click) a pillar to Zip!";

        }

        

        if (!tutorial.empty()) {

            m_TextRenderer->RenderTextCentered(tutorial, w/2, 50, {255, 255, 255, 200});

        }

    }



    // Crosshair

    m_Crosshair->Render(w/2 - 16, h/2 - 16);

    

    // Bow with Sway and Bob

    auto* weapon = m_Registry.GetComponent<WeaponComponent>(m_PlayerEntity);

    if (weapon) {

        Texture* bowTex = weapon->isDrawing ? m_BowDraw.get() : m_BowIdle.get();

        

        int bowW = 400;

        int bowH = 400;

        

        // Sway based on mouse movement would be better, but for now we use a timer

        float swayX = sin(m_SwayTimer * 0.5f) * 15.0f;

        float swayY = cos(m_SwayTimer) * 10.0f;

        

        int bowX = w/2 - bowW/2 + (int)swayX;

        int bowY = h - bowH + 50 + (int)(sin(m_BobTimer) * 15) + (int)swayY;

        

        if (weapon->isDrawing) {

            bowX += (rand() % 4);

            bowY += (rand() % 4);

            // Pull bow back more

            bowY += 20;

        }



        if (m_ShakeTimer > 0) {

            bowX += (int)(((rand() % 100) / 50.0f - 1.0f) * m_ShakeIntensity * 100.0f);

            bowY += (int)(((rand() % 100) / 50.0f - 1.0f) * m_ShakeIntensity * 100.0f);

        }

        

        bowTex->Render(bowX, bowY, bowW, bowH);

    }

}
