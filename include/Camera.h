#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    float yaw = -90.0f;
    float pitch = 0.0f;
    float distance = 20.0f;

    glm::mat4 getViewMatrix() const;
};

