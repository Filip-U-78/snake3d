#include "../include/OrbitCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

OrbitCamera::OrbitCamera(float radius, glm::vec3 target)
    : Radius(radius), Target(target), Yaw(-90.0f), Pitch(-20.0f) {
}

void OrbitCamera::SetMode(CameraMode mode) {
    if (Mode != mode) {
        Mode = mode;
        firstFollowFrame = true; // reset smooth follow
    }
}

void OrbitCamera::SetDeltaTime(float dt) {
    deltaTime = dt;
}

void OrbitCamera::ProcessMouseMovement(float xoffset, float yoffset) {
    if (Mode != CameraMode::Orbit && Mode != CameraMode::FirstPerson)
        return;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (Pitch > 89.0f)  Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;
}

glm::mat4 OrbitCamera::GetViewMatrix(
    const glm::vec3& headPos,
    const glm::vec3& headDir,
    const glm::vec3& headUp
) {
    // ===== ORBIT =====
    if (Mode == CameraMode::Orbit) {
        glm::vec3 pos;
        pos.x = Radius * cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
        pos.y = Radius * sin(glm::radians(Pitch));
        pos.z = Radius * cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));
        return glm::lookAt(pos + Target, Target, glm::vec3(0, 1, 0));
    }

    // ===== FIRST PERSON =====
    if (Mode == CameraMode::FirstPerson) {
        glm::vec3 dir;
        dir.x = cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
        dir.y = sin(glm::radians(Pitch));
        dir.z = cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));
        return glm::lookAt(headPos, headPos + glm::normalize(dir), headUp);
    }

    // ===== FOLLOW (TPP + SMOOTH) =====
    if (Mode == CameraMode::Follow) {

        // --- PARAMETRY KAMERY ---
        const float distBehind = 2.0f;   // bardzo daleko
        const float distUp = 1.5f;   // wysoko
        const float smoothness = 5.0f;   // im wiêksze, tym szybciej reaguje

        glm::vec3 desiredPos =
            headPos
            - glm::normalize(headDir) * distBehind
            + glm::normalize(headUp) * distUp;

        // pierwsza klatka – bez teleportu
        if (firstFollowFrame) {
            currentPos = desiredPos;
            firstFollowFrame = false;
        }

        // exponential smoothing (niezale¿ne od FPS)
        float alpha = 1.0f - std::exp(-smoothness * deltaTime);
        currentPos = glm::mix(currentPos, desiredPos, alpha);

        return glm::lookAt(currentPos, headPos, headUp);
    }

    return glm::mat4(1.0f);
}
