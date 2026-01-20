#include "../include/Logic.h"

glm::ivec3 randomGridPos(std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(-6, 6);
    return { dist(rng), dist(rng), dist(rng) };
}

void spawnApplesAsNeeded(std::vector<Apple>& apples, int maxApples, std::mt19937& rng) {
    while (apples.size() < (size_t)maxApples) apples.push_back({ randomGridPos(rng) });
    while (apples.size() > (size_t)maxApples) apples.pop_back();
}

void resetGameLogic(std::vector<SnakeSegment>& snake, glm::ivec3& snakeDir, glm::ivec3& snakeUp,
    glm::ivec3& pendingDir, glm::ivec3& pendingUp,
    float& moveTimer, bool& canChangeDir,
    std::vector<Apple>& apples, int maxApples, std::mt19937& rng)
{
    snake.clear();
    snake.push_back({ {0, 0, 0} });
    snakeDir = { 1, 0, 0 };
    snakeUp = { 0, 1, 0 };
    pendingDir = snakeDir;
    pendingUp = snakeUp;
    moveTimer = 0.0f;
    canChangeDir = true;
    apples.clear();
    spawnApplesAsNeeded(apples, maxApples, rng);
}

void applyTurn(TurnAction action, const glm::ivec3& forward, const glm::ivec3& up,
    glm::ivec3& outForward, glm::ivec3& outUp)
{
    const glm::ivec3 right = glm::ivec3(glm::cross(glm::vec3(forward), glm::vec3(up)));
    outForward = forward; outUp = up;
    switch (action) {
    case TurnAction::PitchUp:   outForward = up;       outUp = -forward; break;
    case TurnAction::PitchDown: outForward = -up;      outUp = forward;  break;
    case TurnAction::YawLeft:   outForward = -right;   break;
    case TurnAction::YawRight:  outForward = right;    break;
    default: break;
    }
}