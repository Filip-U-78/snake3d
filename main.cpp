#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// NAGŁÓWKI ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "include/Shader.h"
#include "include/OrbitCamera.h"

// --- LOGIKA STANÓW GRY ---
enum class GameState { Menu, Game, Settings, GameOver };
GameState currentState = GameState::Menu;

// --- STRUKTURY ---
struct SnakeSegment {
    glm::ivec3 position;
};

struct Apple {
    glm::ivec3 position;
};

enum class TurnAction { None, PitchUp, PitchDown, YawLeft, YawRight };

// --- ZMIENNE GLOBALNE ---
std::vector<SnakeSegment> snake;
glm::ivec3 snakeDir(1, 0, 0);
glm::ivec3 snakeUp(0, 1, 0);
glm::ivec3 pendingDir = snakeDir;
glm::ivec3 pendingUp = snakeUp;

bool canChangeDir = true;
float moveTimer = 0.0f;
float moveInterval = 0.2f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
int finalScore = 0; // Zmienna do przechowywania wyniku końcowego

std::vector<Apple> apples;
int maxApples = 3;

std::random_device rd;
std::mt19937 rng(rd());

OrbitCamera camera(3.0f, glm::vec3(0.0f, 0.0f, 0.0f));

// --- FUNKCJE POMOCNICZE ---
static glm::ivec3 randomGridPos() {
    std::uniform_int_distribution<int> dist(-6, 6);
    return { dist(rng), dist(rng), dist(rng) };
}

void spawnApplesAsNeeded() {
    while (apples.size() < (size_t)maxApples) {
        apples.push_back({ randomGridPos() });
    }
    while (apples.size() > (size_t)maxApples) {
        apples.pop_back();
    }
}

void resetGame() {
    snake.clear();
    snake.push_back({ {0, 0, 0} });
    snakeDir = { 1, 0, 0 };
    snakeUp = { 0, 1, 0 };
    pendingDir = snakeDir;
    pendingUp = snakeUp;
    moveTimer = 0.0f;
    canChangeDir = true;

    apples.clear();
    spawnApplesAsNeeded();
}

static void applyTurn(TurnAction action, const glm::ivec3& forward, const glm::ivec3& up,
    glm::ivec3& outForward, glm::ivec3& outUp) {
    const glm::ivec3 right = glm::ivec3(glm::cross(glm::vec3(forward), glm::vec3(up)));
    outForward = forward;
    outUp = up;
    switch (action) {
    case TurnAction::PitchUp:   outForward = up;       outUp = -forward; break;
    case TurnAction::PitchDown: outForward = -up;      outUp = forward;  break;
    case TurnAction::YawLeft:   outForward = -right;   break;
    case TurnAction::YawRight:  outForward = right;    break;
    default: break;
    }
}

// --- CALLBACKI ---
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (currentState != GameState::Game) return;
    static float lastX = 400, lastY = 300;
    static bool firstMouse = true;
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    camera.ProcessMouseMovement((float)xpos - lastX, (float)lastY - ypos);
    lastX = (float)xpos; lastY = (float)ypos;
}

// --- RYSOWANIE ---
static glm::mat4 rotationFromForwardUp(const glm::ivec3& forward, const glm::ivec3& up) {
    glm::vec3 f = glm::normalize(glm::vec3(forward));
    glm::vec3 u = glm::normalize(glm::vec3(up));
    glm::vec3 r = glm::normalize(glm::cross(f, u));
    glm::mat4 rot(1.0f);
    rot[0] = glm::vec4(f, 0.0f); rot[1] = glm::vec4(r, 0.0f); rot[2] = glm::vec4(u, 0.0f);
    return rot;
}

void drawSnakeSegment(const glm::ivec3& pos, const glm::ivec3& f, const glm::ivec3& u, bool isHead,
    Shader& shader, unsigned int VAO, const glm::mat4& view, const glm::mat4& projection) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos) * (1.0f / 13.0f));
    model *= rotationFromForwardUp(f, u);
    model = glm::scale(model, glm::vec3(1.0f / 13.0f));

    glm::mat4 mvp = projection * view * model;
    shader.setMat4("MVP", mvp);

    glBindVertexArray(VAO);
    if (isHead) {
        shader.setVec4("color", glm::vec4(0.2f, 0.4f, 1.0f, 1.0f)); glDrawArrays(GL_TRIANGLES, 0, 6);
        shader.setVec4("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); glDrawArrays(GL_TRIANGLES, 6, 30);
    }
    else {
        shader.setVec4("color", glm::vec4(0.0f, 0.8f, 0.2f, 1.0f)); glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

void drawApple(const glm::ivec3& pos, Shader& shader, unsigned int VAO, const glm::mat4& view, const glm::mat4& projection) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos) * (1.0f / 13.0f));
    model = glm::scale(model, glm::vec3(1.0f / 13.0f));

    glm::mat4 mvp = projection * view * model;
    shader.setMat4("MVP", mvp);

    shader.setVec4("color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1000, 800, "Snake 3D Extreme", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetCursorPosCallback(window, mouse_callback);
    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark();

    Shader shader("shaders/basic.vert", "shaders/basic.frag");

    float cubeVertices[] = {
        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
        -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
         0.5f, 0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    resetGame();

    while (!glfwWindowShouldClose(window)) {
        float time = (float)glfwGetTime();
        deltaTime = time - lastFrame; lastFrame = time;
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glClearColor(0.04f, 0.04f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)io.DisplaySize.x / (float)io.DisplaySize.y, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        if (currentState == GameState::Menu) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "SNAKE 3D EXTREME");
            ImGui::Separator();
            if (ImGui::Button("START GRY", ImVec2(220, 45))) { resetGame(); currentState = GameState::Game; }
            if (ImGui::Button("USTAWIENIA", ImVec2(220, 45))) currentState = GameState::Settings;
            if (ImGui::Button("WYJDZ", ImVec2(220, 45))) glfwSetWindowShouldClose(window, true);
            ImGui::End();
        }
        else if (currentState == GameState::Settings) {
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::Begin("Ustawienia", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SliderFloat("Predkosc", &moveInterval, 0.05f, 0.5f);
            if (ImGui::SliderInt("Liczba jablek", &maxApples, 1, 10)) { spawnApplesAsNeeded(); }
            ImGui::Separator();
            if (ImGui::Button("POWROT", ImVec2(120, 30))) currentState = GameState::Menu;
            ImGui::End();
        }
        else if (currentState == GameState::GameOver) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::Begin("Game Over", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);

            ImGui::TextColored(ImVec4(1, 0, 0, 1), "KONIEC GRY!");
            ImGui::Text("Twoj wynik: %d", finalScore);
            ImGui::Separator();

            if (ImGui::Button("ZAGRAJ PONOWNIE", ImVec2(220, 45))) {
                resetGame();
                currentState = GameState::Game;
            }
            if (ImGui::Button("MENU GLOWNE", ImVec2(220, 45))) {
                currentState = GameState::Menu;
            }
            ImGui::End();
        }
        else if (currentState == GameState::Game) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) currentState = GameState::Menu;

            // HUD
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 20, io.DisplaySize.y - 20), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
            ImGui::Begin("ScoreHUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
            ImGui::Text("PUNKTY: %d", (int)snake.size() - 1);
            ImGui::End();

            // Logika sterowania
            if (canChangeDir) {
                TurnAction action = TurnAction::None;
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) action = TurnAction::PitchUp;
                else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) action = TurnAction::PitchDown;
                else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) action = TurnAction::YawLeft;
                else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) action = TurnAction::YawRight;

                if (action != TurnAction::None) {
                    glm::ivec3 nD, nU; applyTurn(action, snakeDir, snakeUp, nD, nU);
                    if (nD != -snakeDir) { pendingDir = nD; pendingUp = nU; canChangeDir = false; }
                }
            }

            // Logika ruchu
            moveTimer += deltaTime;
            if (moveTimer >= moveInterval) {
                moveTimer = 0.0f;
                snakeDir = pendingDir; snakeUp = pendingUp;
                glm::ivec3 oldTail = snake.back().position;
                for (int i = (int)snake.size() - 1; i >= 1; --i) snake[i].position = snake[i - 1].position;
                snake[0].position += snakeDir;

                // Sprawdzenie kolizji (ZAMIANA NA GameState::GameOver)
                bool dead = false;
                if (std::abs(snake[0].position.x) > 6 || std::abs(snake[0].position.y) > 6 || std::abs(snake[0].position.z) > 6) dead = true;
                for (size_t i = 1; i < snake.size(); ++i) if (snake[i].position == snake[0].position) dead = true;

                if (dead) {
                    finalScore = (int)snake.size() - 1;
                    currentState = GameState::GameOver;
                }

                for (size_t i = 0; i < apples.size(); ++i) {
                    if (snake[0].position == apples[i].position) {
                        snake.push_back({ oldTail });
                        apples[i].position = randomGridPos();
                    }
                }
                canChangeDir = true;
            }

            // Renderowanie 3D
            shader.use();
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
            shader.setMat4("MVP", projection * view * glm::mat4(1.0f));
            shader.setVec4("color", glm::vec4(1, 1, 1, 0.03f));
            glBindVertexArray(VAO); glDrawArrays(GL_TRIANGLES, 0, 36);
            glDepthMask(GL_TRUE); glDisable(GL_BLEND);

            for (const auto& app : apples) drawApple(app.position, shader, VAO, view, projection);
            for (size_t i = 0; i < snake.size(); ++i) drawSnakeSegment(snake[i].position, snakeDir, snakeUp, (i == 0), shader, VAO, view, projection);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}