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
    // We can't load here without renderer if we haven't stored it, 
    // but Render pass provides renderer. Ideally load upfront.
    // For now we assume textures are loaded via TextureManager externally 
    // or we store renderer in Init.
}

void Raycaster::Render(SDL_Renderer* ren, const Camera& cam, const Map& map, Registry& reg) {
    // Update size using logical size if available, otherwise output size
    int w, h;
    SDL_RenderGetLogicalSize(ren, &w, &h);
    if (w == 0 || h == 0) {
        SDL_GetRendererOutputSize(ren, &w, &h);
    }

    if (w != m_ScreenWidth || h != m_ScreenHeight) {
        m_ScreenWidth = w;
        m_ScreenHeight = h;
        m_ZBuffer.resize(w);
    }
    
    // Clear screen (ceiling and floor)
    // Ceiling (Top half) - Light Blue or similar
    SDL_SetRenderDrawColor(ren, 135, 206, 235, 255);
    SDL_Rect ceilRect = {0, 0, w, h/2 + (int)cam.pitch};
    SDL_RenderFillRect(ren, &ceilRect);
    
    // Floor (Bottom half) - Dark Green or Gray
    SDL_SetRenderDrawColor(ren, 50, 50, 50, 255);
    SDL_Rect floorRect = {0, h/2 + (int)cam.pitch, w, h - (h/2 + (int)cam.pitch)};
    SDL_RenderFillRect(ren, &floorRect);

    RenderWalls(ren, cam, map);
    RenderSprites(ren, cam, map, reg);
    
    // UI / Overlay (Bow) would be drawn separately after this
}

void Raycaster::RenderWalls(SDL_Renderer* ren, const Camera& cam, const Map& map) {
    double posX = cam.x;
    double posY = cam.y;
    double dirX = std::cos(cam.yaw);
    double dirY = std::sin(cam.yaw);
    double planeX = -0.66 * dirY; // FOV 66
    double planeY = 0.66 * dirX;
    
    int w = m_ScreenWidth;
    int h = m_ScreenHeight;
    
    // Ensure textures are loaded
    // Hardcoding for prototype
    static std::shared_ptr<Texture> texBrick = TextureManager::LoadTexture(ren, "assets/wall_brick.png");
    static std::shared_ptr<Texture> texMoss = TextureManager::LoadTexture(ren, "assets/wall_mossy.png");
    
    for (int x = 0; x < w; x++) {
        double cameraX = 2 * x / (double)w - 1;
        double rayDirX = dirX + planeX * cameraX;
        double rayDirY = dirY + planeY * cameraX;
        
        int mapX = int(posX);
        int mapY = int(posY);
        
        double sideDistX, sideDistY;
        
        double deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1 / rayDirX);
        double deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1 / rayDirY);
        double perpWallDist;
        
        int stepX, stepY;
        int hit = 0;
        int side; // 0 for NS, 1 for EW
        
        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (posX - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0 - posX) * deltaDistX;
        }
        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (posY - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0 - posY) * deltaDistY;
        }
        
        // DDA
        while (hit == 0) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            if (map.Get(mapX, mapY) > 0) hit = 1;
        }
        
        if (side == 0) perpWallDist = (sideDistX - deltaDistX);
        else           perpWallDist = (sideDistY - deltaDistY);
        
        m_ZBuffer[x] = perpWallDist; // Store for sprites
        
        // Calculate height
        int lineHeight = (int)(h / perpWallDist);
        
        // Calculate start and end
        // Horizon logic + pitch
        int horizon = h / 2 + (int)cam.pitch;
        
        // Vertical offset for jumping (cam.z)
        // Walls are world z=0 to z=1. Camera is at cam.z.
        // Standard eye height is 0.5 (middle of the wall).
        int drawStart = horizon - (int)((1.0f - cam.z) * lineHeight);
        int drawEnd = horizon + (int)(cam.z * lineHeight);
        
        // Texture selection
        int texNum = map.Get(mapX, mapY);
        Texture* tex = (texNum == 2) ? texMoss.get() : texBrick.get();
        if (!tex) tex = texBrick.get();
        
        // Texture X calc
        double wallX;
        if (side == 0) wallX = posY + perpWallDist * rayDirY;
        else           wallX = posX + perpWallDist * rayDirX;
        wallX -= floor(wallX);
        
        int texX = int(wallX * double(tex->GetWidth()));
        if (side == 0 && rayDirX > 0) texX = tex->GetWidth() - texX - 1;
        if (side == 1 && rayDirY < 0) texX = tex->GetWidth() - texX - 1;
        
        // Draw Strip
        SDL_Rect srcRect = {texX, 0, 1, tex->GetHeight()};
        
        // Darken side walls
        if (side == 1) tex->SetColorMod(150, 150, 150);
        else tex->SetColorMod(255, 255, 255);
        
        // Distance Shading (Fog)
        Uint8 r, g, b;
        tex->GetColorMod(&r, &g, &b);
        float shadow = 1.0f / (1.0f + perpWallDist * 0.1f); // Linear-ish fog
        if (shadow > 1.0f) shadow = 1.0f;
        if (shadow < 0.1f) shadow = 0.1f;
        
        // Apply fog color (mix with background color)
        SDL_Color fogColor = {50, 50, 60, 255};
        r = (Uint8)(r * shadow + fogColor.r * (1.0f - shadow));
        g = (Uint8)(g * shadow + fogColor.g * (1.0f - shadow));
        b = (Uint8)(b * shadow + fogColor.b * (1.0f - shadow));
        
        tex->SetColorMod(r, g, b);
        
        // Render
        tex->RenderRect(x, drawStart, &srcRect, 1, drawEnd - drawStart);
    }
}

void Raycaster::RenderSprites(SDL_Renderer* ren, const Camera& cam, const Map& map, Registry& reg) {
    // Gather sprites and particles
    struct DrawableSprite {
        double dist;
        Transform3DComponent* trans;
        BillboardComponent* bill;
        ParticleComponent* part; // Optional
    };
    
    std::vector<DrawableSprite> sprites;
    
    // Regular billboards
    auto& billboards = reg.View<BillboardComponent>();
    for (auto& pair : billboards) {
        Entity entity = pair.first;
        if (reg.HasComponent<Transform3DComponent>(entity)) {
            auto* t = reg.GetComponent<Transform3DComponent>(entity);
            double dx = t->x - cam.x;
            double dy = t->y - cam.y;
            double dist = dx*dx + dy*dy;
            sprites.push_back({dist, t, &pair.second, nullptr});
        }
    }

    // Particles (drawn as colored squares or small textures)
    auto& particles = reg.View<ParticleComponent>();
    for (auto& pair : particles) {
        Entity entity = pair.first;
        if (reg.HasComponent<Transform3DComponent>(entity)) {
            auto* t = reg.GetComponent<Transform3DComponent>(entity);
            double dx = t->x - cam.x;
            double dy = t->y - cam.y;
            double dist = dx*dx + dy*dy;
            sprites.push_back({dist, t, nullptr, &pair.second});
        }
    }
    
    // Sort far to near
    std::sort(sprites.begin(), sprites.end(), [](const DrawableSprite& a, const DrawableSprite& b) {
        return a.dist > b.dist; 
    });
    
    double dirX = std::cos(cam.yaw);
    double dirY = std::sin(cam.yaw);
    double planeX = -0.66 * dirY;
    double planeY = 0.66 * dirX;
    
    int w = m_ScreenWidth;
    int h = m_ScreenHeight;
    
    for (const auto& s : sprites) {
        double spriteX = s.trans->x - cam.x;
        double spriteY = s.trans->y - cam.y;
        
        double invDet = 1.0 / (planeX * dirY - dirX * planeY);
        double transformX = invDet * (dirY * spriteX - dirX * spriteY);
        double transformY = invDet * (-planeY * spriteX + planeX * spriteY); 
        
        if (transformY <= 0.1) continue; 
        
        int spriteScreenX = int((w / 2) * (1 + transformX / transformY));
        
        float scale = s.bill ? s.bill->scale : (s.part ? s.part->size * 0.05f : 1.0f);
        int spriteHeight = abs(int(h / transformY)) * scale;
        int horizon = h / 2 + (int)cam.pitch;
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
        
        // Fog for sprites
        float shadow = 1.0f / (1.0f + transformY * 0.1f);
        if (shadow > 1.0f) shadow = 1.0f;
        if (shadow < 0.1f) shadow = 0.1f;
        SDL_Color fogColor = {50, 50, 60, 255};

        if (s.bill) {
            Texture* tex = s.bill->texture.get();
            if (!tex) continue;
            
            Uint8 r = 255, g = 255, b = 255;
            r = (Uint8)(r * shadow + fogColor.r * (1.0f - shadow));
            g = (Uint8)(g * shadow + fogColor.g * (1.0f - shadow));
            b = (Uint8)(b * shadow + fogColor.b * (1.0f - shadow));
            tex->SetColorMod(r, g, b);
            
            for (int stripe = clipStartX; stripe < clipEndX; stripe++) {
                if (transformY < m_ZBuffer[stripe]) {
                    int texX = int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * tex->GetWidth() / spriteWidth) / 256;
                    if (texX < 0) texX = 0; 
                    if (texX >= tex->GetWidth()) texX = tex->GetWidth() - 1;
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
                if (transformY < m_ZBuffer[stripe]) {
                    SDL_RenderDrawLine(ren, stripe, drawStartY, stripe, drawEndY);
                }
            }
        }
    }
}

}
