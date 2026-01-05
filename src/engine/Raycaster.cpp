#include "Raycaster.h"
#include "Components.h"
#include "TextureManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace PixelsEngine {

Raycaster::Raycaster() : m_ScreenWidth(0), m_ScreenHeight(0) {}

Raycaster::~Raycaster() {}

void Raycaster::Init(SDL_Renderer *ren) {
  int w, h;
  SDL_GetRendererOutputSize(ren, &w, &h);
  m_ScreenWidth = w;
  m_ScreenHeight = h;
  m_ZBuffer.resize(w);
}

void Raycaster::LoadTexture(int id, const std::string &path) {}

void Raycaster::Render(SDL_Renderer *ren, const Camera &cam, const Map &map,
                       Registry &reg, float roll) {
  int w, h;
  SDL_RenderGetLogicalSize(ren, &w, &h);
  if (w == 0 || h == 0)
    SDL_GetRendererOutputSize(ren, &w, &h);

  if (w != m_ScreenWidth || h != m_ScreenHeight) {
    m_ScreenWidth = w;
    m_ScreenHeight = h;
    m_ZBuffer.resize(w);
  }

  // 1. Procedural Parallax Sky (Daytime)
  float skyOffset = (cam.yaw / (2.0f * M_PI)) * w * 2.0f;
  for (int x = 0; x < w; x++) {
    int pixelX = (x + (int)skyOffset) % w;
    float grad = (float)pixelX / w;

    // Sky Gradient (Cyan to Blue)
    SDL_SetRenderDrawColor(ren, 135, 206, 235, 255); // Sky Blue
    SDL_RenderDrawLine(ren, x, 0, x, h / 2 + (int)cam.pitch);

    // Simple Sun (Fixed direction)
    float sunDir = 1.0f; // Approx angle
    float angle = atan2(sin(cam.yaw), cos(cam.yaw)); // Current yaw -PI to PI
    // Mapping is complex, simplified: just draw a fixed sun in skybox texture logic
    // Or just a bright gradient at top
    // Let's stick to gradient for now, maybe add a circle later if needed.
    // Horizon glow (White)
    SDL_SetRenderDrawColor(ren, 200, 220, 255, 255);
    SDL_RenderDrawPoint(ren, x, h / 2 + (int)cam.pitch);
  }

  // Dynamic Ambient Pulse (Global light)
  float pulse = 0.95f + sin(SDL_GetTicks() * 0.002f) * 0.05f;

  // Floor Gradient & Special Tiles
  double dirX = std::cos(cam.yaw);
  double dirY = std::sin(cam.yaw);
  double planeX = -0.66 * dirY;
  double planeY = 0.66 * dirX;

  for (int y = h / 2 + (int)cam.pitch + 1; y < h; y++) {
    float rowDist = (cam.z * h) / (y - h / 2 - (int)cam.pitch);
    // Abyss depth (Ground is at -20.0, camera at cam.z)
    float abyssDist = ((cam.z + 20.0f) * h) / (y - h / 2 - (int)cam.pitch);
    
    double rayDirX0 = dirX - planeX;
    double rayDirY0 = dirY - planeY;
    double rayDirX1 = dirX + planeX;
    double rayDirY1 = dirY + planeY;

    double floorStepX = rowDist * (rayDirX1 - rayDirX0) / w;
    double floorStepY = rowDist * (rayDirY1 - rayDirY0) / w;
    double floorX = cam.x + rowDist * rayDirX0;
    double floorY = cam.y + rowDist * rayDirY0;

    for (int x = 0; x < w; x++) {
      int cellX = (int)(floorX);
      int cellY = (int)(floorY);
      int tile = map.Get(cellX, cellY);

      if (tile == 4) {
          // Abyss Rendering (Parallax)
          // Calculate deep floor position
          // We need ray direction for this pixel
          double cameraX = 2 * x / (double)w - 1;
          double rayDirX = dirX + planeX * cameraX;
          double rayDirY = dirY + planeY * cameraX;
          
          double deepX = cam.x + abyssDist * rayDirX;
          double deepY = cam.y + abyssDist * rayDirY;
          
          // Checkerboard pattern for "streets" below
          int streetX = (int)(deepX);
          int streetY = (int)(deepY);
          bool dark = (streetX + streetY) % 2 == 0;
          
          Uint8 r = 30, g = 30, b = 35;
          if (dark) { r = 20; g = 20; b = 25; }
          
          // Abyss Shadow (Darker as it goes down? Actually uniform darkness implies depth here)
          SDL_SetRenderDrawColor(ren, r, g, b, 255);
          SDL_RenderDrawPoint(ren, x, y);
      } else {
          Uint8 r = 100, g = 100, b = 110; // Concrete Gray (Rooftop)
          if (tile == 3) {
            r = 0;
            g = 200;
            b = 200; // Jump Pad
          } 
    
          float shadow = (float)(y - (h / 2 + (int)cam.pitch)) / (h / 2);
          shadow *= pulse;
    
          SDL_SetRenderDrawColor(ren, (Uint8)(r * shadow), (Uint8)(g * shadow),
                                 (Uint8)(b * shadow), 255);
          SDL_RenderDrawPoint(ren, x, y);
      }

      floorX += floorStepX;
      floorY += floorStepY;
    }
  }

  // Add special coloring for lava area? In a raycaster we'd need floor casting.
  // Let's just implement a faster "strip" based floor color for now if we can't
  // do full floor casting.

  RenderWalls(ren, cam, map, roll);
  RenderSprites(ren, cam, map, reg, roll);

  // 3. Post-Process: Vignette
  // We draw a soft darkened border
  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
  for (int i = 0; i < 4; i++) {
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 40 - (i * 10));
    SDL_Rect r = {i * 10, i * 10, w - i * 20, h - i * 20};
    // This is a bit expensive with Rects, let's just do one thick subtle one
  }
  // Efficient Vignette
  SDL_SetRenderDrawColor(ren, 0, 0, 0, 60);
  // Left
  SDL_Rect vL = {0, 0, 80, h};
  SDL_RenderFillRect(ren, &vL);
  // Right
  SDL_Rect vR = {w - 80, 0, 80, h};
  SDL_RenderFillRect(ren, &vR);
  // Top/Bottom (Subtle)
  SDL_SetRenderDrawColor(ren, 0, 0, 0, 40);
  SDL_Rect vT = {0, 0, w, 40};
  SDL_RenderFillRect(ren, &vT);
  SDL_Rect vB = {0, h - 40, w, 40};
  SDL_RenderFillRect(ren, &vB);
  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
}

void Raycaster::RenderWalls(SDL_Renderer *ren, const Camera &cam,
                            const Map &map, float roll) {
  double posX = cam.x;
  double posY = cam.y;
  double dirX = std::cos(cam.yaw);
  double dirY = std::sin(cam.yaw);
  double planeX = -0.66 * dirY;
  double planeY = 0.66 * dirX;

  int w = m_ScreenWidth;
  int h = m_ScreenHeight;
  static std::shared_ptr<Texture> texBrick =
      TextureManager::LoadTexture(ren, "assets/wall_brick.png");
  static std::shared_ptr<Texture> texMoss =
      TextureManager::LoadTexture(ren, "assets/wall_mossy.png");

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
    int stepX, stepY, hit = 0, side;

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
      int tile = map.Get(mapX, mapY);
      if (tile == 1 || tile == 2)
        hit = 1;
    }

    if (side == 0)
      perpWallDist = (sideDistX - deltaDistX);
    else
      perpWallDist = (sideDistY - deltaDistY);

    m_ZBuffer[x] = perpWallDist;
    int lineHeight = (int)(h / perpWallDist);

    float rollOffset = (x - w / 2) * (roll * 0.02f);
    int horizon = h / 2 + (int)cam.pitch + (int)rollOffset;

    int drawStart = horizon - (int)((1.0f - cam.z) * lineHeight);
    int drawEnd = horizon + (int)(cam.z * lineHeight);

    int texNum = map.Get(mapX, mapY);
    Texture *tex = (texNum == 2) ? texMoss.get() : texBrick.get();
    if (!tex)
      tex = texBrick.get();

    double wallX;
    if (side == 0)
      wallX = posY + perpWallDist * rayDirY;
    else
      wallX = posX + perpWallDist * rayDirX;
    wallX -= floor(wallX);

    int texX = int(wallX * double(tex->GetWidth()));
    if (side == 0 && rayDirX > 0)
      texX = tex->GetWidth() - texX - 1;
    if (side == 1 && rayDirY < 0)
      texX = tex->GetWidth() - texX - 1;

    SDL_Rect srcRect = {texX, 0, 1, tex->GetHeight()};
    if (side == 1)
      tex->SetColorMod(150, 150, 150);
    else
      tex->SetColorMod(255, 255, 255);

    Uint8 r, g, b;
    tex->GetColorMod(&r, &g, &b);
    float shadow = 1.0f / (1.0f + perpWallDist * 0.1f);
    shadow = std::max(0.1f, std::min(1.0f, shadow));
    SDL_Color fogColor = {180, 200, 220, 255};
    r = (Uint8)(r * shadow + fogColor.r * (1.0f - shadow));
    g = (Uint8)(g * shadow + fogColor.g * (1.0f - shadow));
    b = (Uint8)(b * shadow + fogColor.b * (1.0f - shadow));
    tex->SetColorMod(r, g, b);

    // Render Main Wall
    tex->RenderRect(x, drawStart, &srcRect, 1, drawEnd - drawStart);

    // 4. Corner Shading (Fake AO)
    // Darken the top and bottom of the wall slightly more to ground it
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_MOD);
    SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
    SDL_RenderDrawPoint(ren, x, drawStart);
    SDL_RenderDrawPoint(ren, x, drawStart + 1);
    SDL_RenderDrawPoint(ren, x, drawEnd - 1);
    SDL_RenderDrawPoint(ren, x, drawEnd - 2);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
  }
}

void Raycaster::RenderSprites(SDL_Renderer *ren, const Camera &cam,
                              const Map &map, Registry &reg, float roll) {
  struct DrawableSprite {
    double dist;
    Transform3DComponent *trans;
    BillboardComponent *bill;
    ParticleComponent *part;
  };
  std::vector<DrawableSprite> sprites;
  auto &billboards = reg.View<BillboardComponent>();
  for (auto &pair : billboards) {
    if (reg.HasComponent<Transform3DComponent>(pair.first)) {
      auto *t = reg.GetComponent<Transform3DComponent>(pair.first);
      double dx = t->x - cam.x;
      double dy = t->y - cam.y;
      sprites.push_back({dx * dx + dy * dy, t, &pair.second, nullptr});
    }
  }
  auto &particles = reg.View<ParticleComponent>();
  for (auto &pair : particles) {
    if (reg.HasComponent<Transform3DComponent>(pair.first)) {
      auto *t = reg.GetComponent<Transform3DComponent>(pair.first);
      double dx = t->x - cam.x;
      double dy = t->y - cam.y;
      sprites.push_back({dx * dx + dy * dy, t, nullptr, &pair.second});
    }
  }
  std::sort(sprites.begin(), sprites.end(),
            [](const DrawableSprite &a, const DrawableSprite &b) {
              return a.dist > b.dist;
            });

  double dirX = std::cos(cam.yaw);
  double dirY = std::sin(cam.yaw);
  double planeX = -0.66 * dirY;
  double planeY = 0.66 * dirX;
  int w = m_ScreenWidth;
  int h = m_ScreenHeight;
  for (const auto &s : sprites) {
    double spriteX = s.trans->x - cam.x;
    double spriteY = s.trans->y - cam.y;
    double invDet = 1.0 / (planeX * dirY - dirX * planeY);
    double transformX = invDet * (dirY * spriteX - dirX * spriteY);
    double transformY = invDet * (-planeY * spriteX + planeX * spriteY);
    if (transformY <= 0.1)
      continue;

    int spriteScreenX = int((w / 2) * (1 + transformX / transformY));
    float scale =
        s.bill ? s.bill->scale : (s.part ? s.part->size * 0.05f : 1.0f);
    int spriteHeight = abs(int(h / transformY)) * scale;

    float rollOffset = (spriteScreenX - w / 2) * (roll * 0.02f);
    int horizon = h / 2 + (int)cam.pitch + (int)rollOffset;
    double heightDiff = (s.trans->z - (cam.z - 0.5));
    int vMoveScreen = int(heightDiff * h / transformY);

    int drawStartY = -spriteHeight / 2 + horizon - vMoveScreen;
    int drawEndY = spriteHeight / 2 + horizon - vMoveScreen;
    int spriteWidth = abs(int(h / transformY)) * scale;
    int drawStartX = -spriteWidth / 2 + spriteScreenX;
    int drawEndX = spriteWidth / 2 + spriteScreenX;
    if (drawStartX >= w || drawEndX < 0)
      continue;
    int clipStartX = std::max(0, drawStartX);
    int clipEndX = std::min(w - 1, drawEndX);

    float shadow = 1.0f / (1.0f + transformY * 0.1f);
    shadow = std::max(0.1f, std::min(1.0f, shadow));
    SDL_Color fogColor = {180, 200, 220, 255}; // Match daylight sky
    if (s.bill) {
      Texture *tex = s.bill->texture.get();
      if (!tex)
        continue;
      Uint8 r = 255, g = 255, b = 255;
      r = (Uint8)(r * shadow + fogColor.r * (1.0f - shadow));
      g = (Uint8)(g * shadow + fogColor.g * (1.0f - shadow));
      b = (Uint8)(b * shadow + fogColor.b * (1.0f - shadow));
      tex->SetColorMod(r, g, b);
      for (int stripe = clipStartX; stripe < clipEndX; stripe++) {
        if (transformY < m_ZBuffer[stripe]) {
          int texX = int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) *
                         tex->GetWidth() / spriteWidth) /
                     256;
          texX = std::max(0, std::min(tex->GetWidth() - 1, texX));
          SDL_Rect srcRect = {texX, 0, 1, tex->GetHeight()};
          tex->RenderRect(stripe, drawStartY, &srcRect, 1,
                          drawEndY - drawStartY);
        }
      }
    } else if (s.part) {
      SDL_Color c = s.part->color;
      c.r = (Uint8)(c.r * shadow + fogColor.r * (1.0f - shadow));
      c.g = (Uint8)(c.g * shadow + fogColor.g * (1.0f - shadow));
      c.b = (Uint8)(c.b * shadow + fogColor.b * (1.0f - shadow));
      SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
      for (int stripe = clipStartX; stripe < clipEndX; stripe++) {
        if (transformY < m_ZBuffer[stripe])
          SDL_RenderDrawLine(ren, stripe, drawStartY, stripe, drawEndY);
      }
    }
  }
}

} // namespace PixelsEngine