#include "Raycaster.h"
#include "TextureManager.h"
#include "Components.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace PixelsEngine {

Raycaster::Raycaster() : m_ScreenWidth(0), m_ScreenHeight(0) {}

Raycaster::~Raycaster() {}

void Raycaster::Init(SDL_Renderer* ren) {
    int w, h;
    SDL_GetRendererOutputSize(ren, &w, &h);
    m_ScreenWidth = w;
    m_ScreenHeight = h;
    m_ZBuffer.resize(w);
}

void Raycaster::LoadTexture(int id, const std::string& path) {
}

void Raycaster::Render(SDL_Renderer* ren, const Camera& cam, const Map& map, Registry& reg, float roll) {
    int w, h;
    SDL_RenderGetLogicalSize(ren, &w, &h);
    if (w == 0 || h == 0) SDL_GetRendererOutputSize(ren, &w, &h);

    if (w != m_ScreenWidth || h != m_ScreenHeight) {
        m_ScreenWidth = w;
        m_ScreenHeight = h;
        m_ZBuffer.resize(w);
    }
    
    // Ceiling
    SDL_SetRenderDrawColor(ren, 135, 206, 235, 255);
    SDL_Rect ceilRect = {0, 0, w, h};
    SDL_RenderFillRect(ren, &ceilRect);
    
    // Floor Gradient
    for (int y = h/2 + (int)cam.pitch; y < h; y++) {
        float shadow = (float)(y - (h/2 + (int)cam.pitch)) / (h/2);
        SDL_SetRenderDrawColor(ren, (Uint8)(50 * shadow), (Uint8)(70 * shadow), (Uint8)(50 * shadow), 255);
        SDL_RenderDrawLine(ren, 0, y, w, y);
    }

    RenderWalls(ren, cam, map, roll);
    RenderSprites(ren, cam, map, reg, roll);
}

void Raycaster::RenderWalls(SDL_Renderer* ren, const Camera& cam, const Map& map, float roll) {
    double posX = cam.x; double posY = cam.y;
    double dirX = std::cos(cam.yaw); double dirY = std::sin(cam.yaw);
    double planeX = -0.66 * dirY; double planeY = 0.66 * dirX;
    
    int w = m_ScreenWidth; int h = m_ScreenHeight;
    static std::shared_ptr<Texture> texBrick = TextureManager::LoadTexture(ren, "assets/wall_brick.png");
    static std::shared_ptr<Texture> texMoss = TextureManager::LoadTexture(ren, "assets/wall_mossy.png");
    
    for (int x = 0; x < w; x++) {
        double cameraX = 2 * x / (double)w - 1;
        double rayDirX = dirX + planeX * cameraX;
        double rayDirY = dirY + planeY * cameraX;
        
        int mapX = int(posX); int mapY = int(posY);
        double sideDistX, sideDistY;
        double deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1 / rayDirX);
        double deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1 / rayDirY);
        double perpWallDist;
        int stepX, stepY, hit = 0, side;
        
        if (rayDirX < 0) { stepX = -1; sideDistX = (posX - mapX) * deltaDistX; }
        else { stepX = 1; sideDistX = (mapX + 1.0 - posX) * deltaDistX; }
        if (rayDirY < 0) { stepY = -1; sideDistY = (posY - mapY) * deltaDistY; }
        else { stepY = 1; sideDistY = (mapY + 1.0 - posY) * deltaDistY; }
        
        while (hit == 0) {
            if (sideDistX < sideDistY) { sideDistX += deltaDistX; mapX += stepX; side = 0; }
            else { sideDistY += deltaDistY; mapY += stepY; side = 1; }
            if (map.Get(mapX, mapY) > 0) hit = 1;
        }
        
        if (side == 0) perpWallDist = (sideDistX - deltaDistX);
        else           perpWallDist = (sideDistY - deltaDistY);
        
        m_ZBuffer[x] = perpWallDist;
        int lineHeight = (int)(h / perpWallDist);
        
        float rollOffset = (x - w/2) * (roll * 0.02f);
        int horizon = h / 2 + (int)cam.pitch + (int)rollOffset;
        
        int drawStart = horizon - (int)((1.0f - cam.z) * lineHeight);
        int drawEnd = horizon + (int)(cam.z * lineHeight);
        
        int texNum = map.Get(mapX, mapY);
        Texture* tex = (texNum == 2) ? texMoss.get() : texBrick.get();
        if (!tex) tex = texBrick.get();
        
        double wallX;
        if (side == 0) wallX = posY + perpWallDist * rayDirY;
        else           wallX = posX + perpWallDist * rayDirX;
        wallX -= floor(wallX);
        
        int texX = int(wallX * double(tex->GetWidth()));
        if (side == 0 && rayDirX > 0) texX = tex->GetWidth() - texX - 1;
        if (side == 1 && rayDirY < 0) texX = tex->GetWidth() - texX - 1;
        
        SDL_Rect srcRect = {texX, 0, 1, tex->GetHeight()};
        if (side == 1) tex->SetColorMod(150, 150, 150); else tex->SetColorMod(255, 255, 255);
        
        Uint8 r, g, b; tex->GetColorMod(&r, &g, &b);
        float shadow = 1.0f / (1.0f + perpWallDist * 0.1f);
        shadow = std::max(0.1f, std::min(1.0f, shadow));
        SDL_Color fogColor = {50, 50, 60, 255};
        r = (Uint8)(r * shadow + fogColor.r * (1.0f - shadow));
        g = (Uint8)(g * shadow + fogColor.g * (1.0f - shadow));
        b = (Uint8)(b * shadow + fogColor.b * (1.0f - shadow));
        tex->SetColorMod(r, g, b);
        tex->RenderRect(x, drawStart, &srcRect, 1, drawEnd - drawStart);
    }
}

void Raycaster::RenderSprites(SDL_Renderer* ren, const Camera& cam, const Map& map, Registry& reg, float roll) {
    struct DrawableSprite {
        double dist; Transform3DComponent* trans; BillboardComponent* bill; ParticleComponent* part;
    };
    std::vector<DrawableSprite> sprites;
    auto& billboards = reg.View<BillboardComponent>();
    for (auto& pair : billboards) {
        if (reg.HasComponent<Transform3DComponent>(pair.first)) {
            auto* t = reg.GetComponent<Transform3DComponent>(pair.first);
            double dx = t->x - cam.x; double dy = t->y - cam.y;
            sprites.push_back({dx*dx + dy*dy, t, &pair.second, nullptr});
        }
    }
    auto& particles = reg.View<ParticleComponent>();
    for (auto& pair : particles) {
        if (reg.HasComponent<Transform3DComponent>(pair.first)) {
            auto* t = reg.GetComponent<Transform3DComponent>(pair.first);
            double dx = t->x - cam.x; double dy = t->y - cam.y;
            sprites.push_back({dx*dx + dy*dy, t, nullptr, &pair.second});
        }
    }
    std::sort(sprites.begin(), sprites.end(), [](const DrawableSprite& a, const DrawableSprite& b) { return a.dist > b.dist; });
    
    double dirX = std::cos(cam.yaw); double dirY = std::sin(cam.yaw);
    double planeX = -0.66 * dirY; double planeY = 0.66 * dirX;
    int w = m_ScreenWidth; int h = m_ScreenHeight;
    for (const auto& s : sprites) {
        double spriteX = s.trans->x - cam.x; double spriteY = s.trans->y - cam.y;
        double invDet = 1.0 / (planeX * dirY - dirX * planeY);
        double transformX = invDet * (dirY * spriteX - dirX * spriteY);
        double transformY = invDet * (-planeY * spriteX + planeX * spriteY); 
        if (transformY <= 0.1) continue; 
        
        int spriteScreenX = int((w / 2) * (1 + transformX / transformY));
        float scale = s.bill ? s.bill->scale : (s.part ? s.part->size * 0.05f : 1.0f);
        int spriteHeight = abs(int(h / transformY)) * scale;
        
        float rollOffset = (spriteScreenX - w/2) * (roll * 0.02f);
        int horizon = h / 2 + (int)cam.pitch + (int)rollOffset;
        double heightDiff = (s.trans->z - (cam.z - 0.5)); 
        int vMoveScreen = int(heightDiff * h / transformY);
        
        int drawStartY = -spriteHeight / 2 + horizon - vMoveScreen; 
        int drawEndY = spriteHeight / 2 + horizon - vMoveScreen;
        int spriteWidth = abs(int(h / transformY)) * scale;
        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        int drawEndX = spriteWidth / 2 + spriteScreenX;
        if (drawStartX >= w || drawEndX < 0) continue;
        int clipStartX = std::max(0, drawStartX);
        int clipEndX = std::min(w - 1, drawEndX);
        
        float shadow = 1.0f / (1.0f + transformY * 0.1f);
        shadow = std::max(0.1f, std::min(1.0f, shadow));
        SDL_Color fogColor = {50, 50, 60, 255};
        if (s.bill) {
            Texture* tex = s.bill->texture.get(); if (!tex) continue;
            Uint8 r = 255, g = 255, b = 255;
            r = (Uint8)(r * shadow + fogColor.r * (1.0f - shadow));
            g = (Uint8)(g * shadow + fogColor.g * (1.0f - shadow));
            b = (Uint8)(b * shadow + fogColor.b * (1.0f - shadow));
            tex->SetColorMod(r, g, b);
            for (int stripe = clipStartX; stripe < clipEndX; stripe++) {
                if (transformY < m_ZBuffer[stripe]) {
                    int texX = int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * tex->GetWidth() / spriteWidth) / 256;
                    texX = std::max(0, std::min(tex->GetWidth() - 1, texX));
                    SDL_Rect srcRect = {texX, 0, 1, tex->GetHeight()};
                    tex->RenderRect(stripe, drawStartY, &srcRect, 1, drawEndY - drawStartY);
                }
            }
        } else if (s.part) {
            SDL_Color c = s.part->color;
            c.r = (Uint8)(c.r * shadow + fogColor.r * (1.0f - shadow));
            c.g = (Uint8)(c.g * shadow + fogColor.g * (1.0f - shadow));
            c.b = (Uint8)(c.b * shadow + fogColor.b * (1.0f - shadow));
            SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
            for (int stripe = clipStartX; stripe < clipEndX; stripe++) {
                if (transformY < m_ZBuffer[stripe]) SDL_RenderDrawLine(ren, stripe, drawStartY, stripe, drawEndY);
            }
        }
    }
}

}