#include "Config.h"

namespace PixelsEngine {
std::unordered_map<GameAction, SDL_Scancode> Config::m_Keybinds;
float Config::m_MouseSensitivity = 0.003f;
} // namespace PixelsEngine
