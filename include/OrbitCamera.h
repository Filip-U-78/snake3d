#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraMode { Orbit, TPP };

class OrbitCamera {
public:
    CameraMode Mode = CameraMode::Orbit;

    OrbitCamera(float radius, glm::vec3 target);

    glm::mat4 GetViewMatrix(
        const glm::vec3& headPos,
        const glm::vec3& headDir,
        const glm::vec3& headUp
    );

    void ProcessMouseMovement(float xoffset, float yoffset);
    void SetMode(CameraMode mode);

    // >>> SMOOTH FOLLOW <<<
    void SetDeltaTime(float dt);

private:
    // orbit
    float Radius;
    float Yaw;
    float Pitch;
    glm::vec3 Target;

    // smooth follow
    glm::vec3 currentPos;
    bool firstFollowFrame = true;
    float deltaTime = 0.016f;
};
