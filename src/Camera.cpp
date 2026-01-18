#include "../include/Camera.h"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 pos;
    pos.x = distance * cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    pos.y = distance * sin(glm::radians(pitch));
    pos.z = distance * sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    return glm::lookAt(pos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
}
