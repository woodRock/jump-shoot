#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include <map>
#include <memory>
#include "Camera.h"
#include "Map.h"
#include "ECS.h"
#include "Texture.h"

namespace PixelsEngine {

class Raycaster {
public:
    Raycaster();
    ~Raycaster();

    void Init(SDL_Renderer* ren);
    void LoadTexture(int id, const std::string& path);
    
    // Main render function
    void Render(SDL_Renderer* ren, const Camera& cam, const Map& map, Registry& reg, float roll = 0.0f);

private:
    void RenderWalls(SDL_Renderer* ren, const Camera& cam, const Map& map, float roll);
    void RenderSprites(SDL_Renderer* ren, const Camera& cam, const Map& map, Registry& reg, float roll);
    void RenderFloorCeiling(SDL_Renderer* ren, const Camera& cam); // Optional/Solid color

    std::map<int, std::shared_ptr<Texture>> m_Textures;
    std::vector<double> m_ZBuffer; // Distance to wall for each column
    
    int m_ScreenWidth;
    int m_ScreenHeight;
};

}
