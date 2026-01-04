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
    // Update size if changed
    int w, h;
    SDL_GetRendererOutputSize(ren, &w, &h);
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
        
        // Cap draw to screen to avoid weird scaling issues or crashes? 
        // SDL handles clipping usually.
        // Render
        tex->RenderRect(x, drawStart, &srcRect, 1, drawEnd - drawStart);
    }
}

void Raycaster::RenderSprites(SDL_Renderer* ren, const Camera& cam, const Map& map, Registry& reg) {
    // Gather sprites
    struct DrawableSprite {
        double dist;
        Transform3DComponent* trans;
        BillboardComponent* bill;
    };
    
    std::vector<DrawableSprite> sprites;
    // Iterate over one component type and check for the other
    auto& billboards = reg.View<BillboardComponent>();
    
    for (auto& pair : billboards) {
        Entity entity = pair.first;
        BillboardComponent& b = pair.second;
        
        if (reg.HasComponent<Transform3DComponent>(entity)) {
            Transform3DComponent* t = reg.GetComponent<Transform3DComponent>(entity);
            
            double dx = t->x - cam.x;
            double dy = t->y - cam.y;
            double dist = dx*dx + dy*dy;
            sprites.push_back({dist, t, &b});
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
        
        // Inverse Camera Matrix
        double invDet = 1.0 / (planeX * dirY - dirX * planeY);
        double transformX = invDet * (dirY * spriteX - dirX * spriteY);
        double transformY = invDet * (-planeY * spriteX + planeX * spriteY); // Depth inside screen
        
        if (transformY <= 0.1) continue; // Behind camera or too close
        
        int spriteScreenX = int((w / 2) * (1 + transformX / transformY));
        
        // Height calculation
        int spriteHeight = abs(int(h / transformY)) * s.bill->scale;
        
        // Vertical position adjust
        // Sprites are usually on floor (z=0) or flying
        // s.trans->z vs cam.z
        // Standard center is horizon.
        int horizon = h / 2 + (int)cam.pitch;
        
        // Sprite base should be at floor...
        // For wall: center is horizon.
        // For sprite with z=0:
        // shift = (s.z - cam.z + 0.5?) 
        // Let's approximate.
        // Move sprite down/up based on z difference
        // If s.z = 0 (floor) and cam.z = 0.5 (eye), sprite is lower.
        
        // (s.trans->z - cam.z) relative height difference
        // Projected height diff = (diff) / transformY * something
        
        double heightDiff = (s.trans->z - (cam.z - 0.5)); // 0 - 0 = 0 (centered?) No.
        
        int vMoveScreen = int(heightDiff * h / transformY);
        
        int drawStartY = -spriteHeight / 2 + horizon - vMoveScreen; 
        int drawEndY = spriteHeight / 2 + horizon - vMoveScreen;
        
        int spriteWidth = abs(int(h / transformY)) * s.bill->scale; // Assuming square ratio
        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        int drawEndX = spriteWidth / 2 + spriteScreenX;
        
        if (drawStartX >= w || drawEndX < 0) continue;
        
        // Clip
        int clipStartX = drawStartX;
        int clipEndX = drawEndX;
        if (drawStartX < 0) clipStartX = 0;
        if (drawEndX >= w) clipEndX = w - 1;
        
        // Render Strips
        Texture* tex = s.bill->texture.get();
        if (!tex) continue; // Should check valid
        
        tex->SetColorMod(255, 255, 255); // Reset color
        
        for (int stripe = clipStartX; stripe < clipEndX; stripe++) {
            if (transformY < m_ZBuffer[stripe]) {
                int texX = int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * tex->GetWidth() / spriteWidth) / 256;
                if (texX < 0) texX = 0; 
                if (texX >= tex->GetWidth()) texX = tex->GetWidth() - 1;
                
                SDL_Rect srcRect = {texX, 0, 1, tex->GetHeight()};
                
                // Draw
                // We don't clip Y here, SDL does.
                tex->RenderRect(stripe, drawStartY, &srcRect, 1, drawEndY - drawStartY);
            }
        }
    }
}

}
