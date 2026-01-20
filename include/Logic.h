#pragma once
#ifndef LOGIC_H
#define LOGIC_H

#include "Shared.h"
#include <vector>
#include <random>

glm::ivec3 randomGridPos(std::mt19937& rng);

void spawnApplesAsNeeded(std::vector<Apple>& apples, int maxApples, std::mt19937& rng);

void resetGameLogic(std::vector<SnakeSegment>& snake, glm::ivec3& snakeDir, glm::ivec3& snakeUp,
    glm::ivec3& pendingDir, glm::ivec3& pendingUp,
    float& moveTimer, bool& canChangeDir,
    std::vector<Apple>& apples, int maxApples, std::mt19937& rng);

void applyTurn(TurnAction action, const glm::ivec3& forward, const glm::ivec3& up,
    glm::ivec3& outForward, glm::ivec3& outUp);

#endif