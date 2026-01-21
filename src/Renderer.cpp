#include "../include/Renderer.h"
#include <glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

// Funkcja pomocnicza (lokalna dla tego pliku)
static glm::mat4 rotationFromForwardUp(const glm::ivec3& forward, const glm::ivec3& up) {
    glm::vec3 f = glm::normalize(glm::vec3(forward));
    glm::vec3 u = glm::normalize(glm::vec3(up));
    glm::vec3 r = glm::normalize(glm::cross(f, u));
    glm::mat4 rot(1.0f);
    rot[0] = glm::vec4(f, 0.0f); rot[1] = glm::vec4(r, 0.0f); rot[2] = glm::vec4(u, 0.0f);
    return rot;
}

void bindCubeTextures(const CubeTextures& texs, Shader& shader) {
    for (int i = 0; i < 6; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, texs.faces[i]);
        std::string name = "textures[" + std::to_string(i) + "]";
        shader.setInt(name.c_str(), i);
    }
}

void drawSnakeSegment(const glm::ivec3& pos, const glm::ivec3& f, const glm::ivec3& u, bool isHead,
    Shader& shader, unsigned int VAO, const CubeTextures& headTex, const CubeTextures& bodyTex,
    const glm::mat4& view, const glm::mat4& projection) {

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos) * (1.0f / 13.0f));
    model *= rotationFromForwardUp(f, u);
    model = glm::scale(model, glm::vec3(1.0f / 13.0f));

    shader.use();
    shader.setMat4("MVP", projection * view * model);
    shader.setBool("useTexture", true);
    shader.setVec4("color", glm::vec4(1.0f));

    if (isHead) bindCubeTextures(headTex, shader);
    else        bindCubeTextures(bodyTex, shader);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawApple(const glm::ivec3& pos, Shader& shader, unsigned int VAO, const CubeTextures& appleTex,
    const glm::mat4& view, const glm::mat4& projection) {

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos) * (1.0f / 13.0f));
    model = glm::scale(model, glm::vec3(1.0f / 13.0f));

    shader.use();
    shader.setMat4("MVP", projection * view * model);
    shader.setBool("useTexture", true);
    shader.setVec4("color", glm::vec4(1.0f));

    bindCubeTextures(appleTex, shader);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawBoundaryGrid(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    constexpr float B = 6.0f;
    constexpr float S = 1.0f / 13.0f;
    glm::vec3 edges[] = {
        {-B,-B,-B},{ B,-B,-B}, { B,-B,-B},{ B, B,-B}, { B, B,-B},{-B, B,-B}, {-B, B,-B},{-B,-B,-B},
        {-B,-B, B},{ B,-B, B}, { B,-B, B},{ B, B, B}, { B, B, B},{-B, B, B}, {-B, B, B},{-B,-B, B},
        {-B,-B,-B},{-B,-B, B}, { B,-B,-B},{ B,-B, B}, { B, B,-B},{ B, B, B}, {-B, B,-B},{-B, B, B}
    };
    unsigned int gVAO, gVBO;
    glGenVertexArrays(1, &gVAO); glGenBuffers(1, &gVBO);
    glBindVertexArray(gVAO); glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(edges), edges, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    shader.use();
    shader.setMat4("MVP", projection * view * glm::scale(glm::mat4(1.0f), glm::vec3(S)));
    shader.setBool("useTexture", false);
    shader.setVec4("color", glm::vec4(0.6f, 0.8f, 1.0f, 0.25f));
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_LINES, 0, 24);
    glDisable(GL_BLEND);
    glDeleteBuffers(1, &gVBO); glDeleteVertexArrays(1, &gVAO);
}