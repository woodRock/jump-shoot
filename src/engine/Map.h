#pragma once
#include <vector>

namespace PixelsEngine {

struct Map {
    static const int WIDTH = 24;
    static const int HEIGHT = 24;
    
    // 0 = empty, >0 = wall texture ID
    int tiles[WIDTH * HEIGHT];

    int Get(int x, int y) const {
        if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return 1; // Out of bounds is wall
        return tiles[y * WIDTH + x];
    }
    
    void Set(int x, int y, int val) {
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
            tiles[y * WIDTH + x] = val;
        }
    }
};

}
