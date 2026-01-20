#include "../include/OrbitCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <cmath> // Potrzebne dla std::exp

OrbitCamera::OrbitCamera(float radius, glm::vec3 target)
    : Radius(radius), Target(target), Yaw(-90.0f), Pitch(-20.0f), Mode(CameraMode::Orbit) {
    firstFollowFrame = true;
}

void OrbitCamera::SetMode(CameraMode mode) {
    if (Mode != mode) {
        Mode = mode;
        firstFollowFrame = true; // reset smooth follow przy zmianie trybu
    }
}

void OrbitCamera::SetDeltaTime(float dt) {
    deltaTime = dt;
}

void OrbitCamera::ProcessMouseMovement(float xoffset, float yoffset) {
    // Myszka dzia³a tylko w trybie Orbit
    if (Mode != CameraMode::Orbit)
        return;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // Blokada kamery góra/dó³
    if (Pitch > 89.0f)  Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;
}

glm::mat4 OrbitCamera::GetViewMatrix(
    const glm::vec3& headPos,
    const glm::vec3& headDir,
    const glm::vec3& headUp
) {
    // ===== TRYB: ORBIT (Kamera wokó³ punktu œrodkowego) =====
    if (Mode == CameraMode::Orbit) {
        glm::vec3 pos;
        pos.x = Radius * cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
        pos.y = Radius * sin(glm::radians(Pitch));
        pos.z = Radius * cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));
        return glm::lookAt(pos + Target, Target, glm::vec3(0, 1, 0));
    }

    // ===== TRYB: TPP (Kamera pod¹¿aj¹ca za wê¿em) =====
    if (Mode == CameraMode::TPP) {

        // --- PARAMETRY KAMERY ---
        const float distBehind = 2.0f;   // odleg³oœæ za g³ow¹
        const float distUp = 1.5f;       // wysokoœæ nad g³ow¹
        const float smoothness = 5.0f;   // wyg³adzanie ruchu

        glm::vec3 desiredPos =
            headPos
            - glm::normalize(headDir) * distBehind
            + glm::normalize(headUp) * distUp;

        // Jeœli to pierwsza klatka w tym trybie, ustaw kamerê natychmiast bez wyg³adzania
        if (firstFollowFrame) {
            currentPos = desiredPos;
            firstFollowFrame = false;
        }

        // P³ynne pod¹¿anie (niezale¿ne od klatek na sekundê)
        float alpha = 1.0f - std::exp(-smoothness * deltaTime);
        currentPos = glm::mix(currentPos, desiredPos, alpha);

        return glm::lookAt(currentPos, headPos, headUp);
    }

    return glm::mat4(1.0f);
}