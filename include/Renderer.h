#ifndef RENDERER_H
#define RENDERER_H

#include <glm/glm.hpp>
#include "Shader.h"
#include "Shared.h"

void bindCubeTextures(const CubeTextures& texs, Shader& shader);

void drawSnakeSegment(const glm::ivec3& pos, const glm::ivec3& f, const glm::ivec3& u, bool isHead,
    Shader& shader, unsigned int VAO, const CubeTextures& headTex, const CubeTextures& bodyTex,
    const glm::mat4& view, const glm::mat4& projection);

void drawApple(const glm::ivec3& pos, Shader& shader, unsigned int VAO, const CubeTextures& appleTex,
    const glm::mat4& view, const glm::mat4& projection);

void drawBoundaryGrid(Shader& shader, const glm::mat4& view, const glm::mat4& projection);

#endif