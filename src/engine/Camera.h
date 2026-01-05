#pragma once

namespace PixelsEngine {

struct Camera {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;     // Height
  float yaw = 0.0f;   // Rotation in radians
  float pitch = 0.0f; // Look up/down offset (in pixels or angle approx)

  int width;
  int height;

  Camera(int w, int h) : width(w), height(h) {}
};

} // namespace PixelsEngine