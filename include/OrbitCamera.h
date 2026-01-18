#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class OrbitCamera {
public:
    float Radius;
    float Yaw;   // kąt poziomy
    float Pitch; // kąt pionowy
    glm::vec3 Target; // środek sześcianu

    OrbitCamera(float radius, glm::vec3 target);

    glm::mat4 GetViewMatrix();
    void ProcessMouseMovement(float xoffset, float yoffset);
};
