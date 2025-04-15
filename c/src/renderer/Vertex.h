#pragma once

#include <glm/glm.hpp>

namespace tewduwu {

// Simple Vertex structure for 2D rendering with texture coordinates
struct TextVertex {
  glm::vec2 pos;
  glm::vec2 texCoord;
};

}  // namespace tewduwu