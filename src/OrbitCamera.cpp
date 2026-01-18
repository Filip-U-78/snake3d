#include "../include/OrbitCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

OrbitCamera::OrbitCamera(float radius, glm::vec3 target)
    : Radius(radius), Yaw(-90.0f), Pitch(0.0f), Target(target) {
}

glm::mat4 OrbitCamera::GetViewMatrix() {
    // Konwersja k?tów do pozycji kamery w kartezja?skim
    float x = Target.x + Radius * cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
    float y = Target.y + Radius * sin(glm::radians(Pitch));
    float z = Target.z + Radius * cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));
    glm::vec3 position(x, y, z);

    return glm::lookAt(position, Target, glm::vec3(0.0f, 1.0f, 0.0f));
}

void OrbitCamera::ProcessMouseMovement(float xoffset, float yoffset) {
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;
}
