#ifndef SHARED_H
#define SHARED_H

#include <glm/glm.hpp>

// Struktury
struct CubeTextures {
    unsigned int faces[6]; // 0:przód, 1:ty³, 2:lewo, 3:prawo, 4:dó³, 5:góra
};

struct SnakeSegment {
    glm::ivec3 position;
};

struct Apple {
    glm::ivec3 position;
};

// Enumy
enum class GameState { Menu, Game, Settings, GameOver };
enum class TurnAction { None, PitchUp, PitchDown, YawLeft, YawRight };

#endif