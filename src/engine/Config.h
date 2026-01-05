#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <unordered_map>

namespace PixelsEngine {

enum class GameAction {
  MoveUp,
  MoveDown,
  MoveLeft,
  MoveRight,
  AttackModifier,
  Inventory,
  Map,
  Character,
  Magic,
  Pause,
  EndTurn,
  Dash,
  Jump,
  Sneak,
  Shove,
  ToggleWeapon,
  ToggleFullScreen
};

class Config {
public:
  static void Init() {
    // Default Keybinds
    m_Keybinds[GameAction::MoveUp] = SDL_SCANCODE_W;
    m_Keybinds[GameAction::MoveDown] = SDL_SCANCODE_S;
    m_Keybinds[GameAction::MoveLeft] = SDL_SCANCODE_A;
    m_Keybinds[GameAction::MoveRight] = SDL_SCANCODE_D;
    m_Keybinds[GameAction::AttackModifier] = SDL_SCANCODE_LSHIFT;
    m_Keybinds[GameAction::Inventory] = SDL_SCANCODE_I;
    m_Keybinds[GameAction::Map] = SDL_SCANCODE_M;
    m_Keybinds[GameAction::Character] = SDL_SCANCODE_O; // Moved from C to O
    m_Keybinds[GameAction::Magic] = SDL_SCANCODE_K;
    m_Keybinds[GameAction::Pause] = SDL_SCANCODE_ESCAPE;
    m_Keybinds[GameAction::EndTurn] = SDL_SCANCODE_SPACE;
    m_Keybinds[GameAction::Jump] = SDL_SCANCODE_Z;
    m_Keybinds[GameAction::Sneak] = SDL_SCANCODE_C;
    m_Keybinds[GameAction::Shove] = SDL_SCANCODE_V;
    m_Keybinds[GameAction::Dash] = SDL_SCANCODE_B;
    m_Keybinds[GameAction::ToggleWeapon] = SDL_SCANCODE_F;
    m_Keybinds[GameAction::ToggleFullScreen] = SDL_SCANCODE_F11;
  }

  static SDL_Scancode GetKeybind(GameAction action) {
    return m_Keybinds[action];
  }

  static void SetKeybind(GameAction action, SDL_Scancode scancode) {
    m_Keybinds[action] = scancode;
  }

  static std::string GetActionName(GameAction action) {
    switch (action) {
    case GameAction::MoveUp:
      return "Move Up";
    case GameAction::MoveDown:
      return "Move Down";
    case GameAction::MoveLeft:
      return "Move Left";
    case GameAction::MoveRight:
      return "Move Right";
    case GameAction::AttackModifier:
      return "Attack (Hold + Click)";
    case GameAction::Inventory:
      return "Inventory";
    case GameAction::Map:
      return "Map";
    case GameAction::Character:
      return "Character Sheet";
    case GameAction::Magic:
      return "Magic";
    case GameAction::Pause:
      return "Pause/Back";
    case GameAction::EndTurn:
      return "End Turn";
    case GameAction::Jump:
      return "Jump";
    case GameAction::Sneak:
      return "Hide/Sneak";
    case GameAction::Shove:
      return "Shove";
    case GameAction::Dash:
      return "Dash";
    case GameAction::ToggleWeapon:
      return "Toggle Melee/Ranged";
    default:
      return "Unknown";
    }
  }

  static float GetMouseSensitivity() { return m_MouseSensitivity; }
  static void SetMouseSensitivity(float s) { m_MouseSensitivity = s; }

private:
  static std::unordered_map<GameAction, SDL_Scancode> m_Keybinds;
  static float m_MouseSensitivity;
};

} // namespace PixelsEngine
