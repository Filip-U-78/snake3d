#include <iostream>
#include <vector>
#include <random>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// TWOJE MODUŁY
#include "include/Shared.h"
#include "include/Shader.h"
#include "include/OrbitCamera.h"
#include "include/Renderer.h"
#include "include/Logic.h"
#include "include/TextureLoader.h"
#include "include/GameUI.h" // <-- NOWY INCLUDE

// Zmienne globalne
GameState currentState = GameState::Menu;
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
int finalScore = 0;

std::vector<Apple> apples;
int maxApples = 3;

std::random_device rd;
std::mt19937 rng(rd());

OrbitCamera camera(3.0f, glm::vec3(0.0f, 0.0f, 0.0f));

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (currentState != GameState::Game) return;
    static float lastX = 400, lastY = 300;
    static bool firstMouse = true;
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    camera.ProcessMouseMovement((float)xpos - lastX, (float)lastY - (float)ypos);
    lastX = (float)xpos; lastY = (float)ypos;
}

// Wrapper do resetowania, żeby kod w main był krótszy
void ResetGameWrapper() {
    resetGameLogic(snake, snakeDir, snakeUp, pendingDir, pendingUp, moveTimer, canChangeDir, apples, maxApples, rng);
}

int main() {
    int speedLevel = 5;       // To przesyłamy do Menu (suwak 1-10)
    float moveInterval = 0.5f;

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1000, 800, "Snake 3D Modular", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glfwSetCursorPosCallback(window, mouse_callback);
    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Shader shader("shaders/basic.vert", "shaders/basic.frag");

    // DANE WIERZCHOŁKÓW
    float cubeVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,   0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,   0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,   0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,   0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 2.0f,  -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 2.0f,  -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 2.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 2.0f,  -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 2.0f,  -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 2.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 3.0f,   0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 3.0f,   0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 3.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 3.0f,   0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 3.0f,   0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 3.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 4.0f,   0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 4.0f,   0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 4.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 4.0f,  -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 4.0f,  -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 4.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 5.0f,   0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 5.0f,   0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 5.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 5.0f,  -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 5.0f,  -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 5.0f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float))); glEnableVertexAttribArray(2);

    CubeTextures appleTex, headTex, bodyTex;
    appleTex.faces[5] = loadTexture("textures/apple_up.jpg");
    appleTex.faces[4] = loadTexture("textures/apple_down.jpg");
    appleTex.faces[0] = appleTex.faces[1] = appleTex.faces[2] = appleTex.faces[3] = loadTexture("textures/apple_side.jpg");

    headTex.faces[5] = loadTexture("textures/snake_head_up.jpg");
    headTex.faces[4] = loadTexture("textures/snake_head_down.jpg");
    headTex.faces[0] = loadTexture("textures/snake_head_front.jpg");
    headTex.faces[1] = headTex.faces[2] = headTex.faces[3] = loadTexture("textures/snake_head_side.jpg");

    bodyTex.faces[5] = loadTexture("textures/snake_body_up.jpg");
    bodyTex.faces[4] = bodyTex.faces[0] = bodyTex.faces[1] = bodyTex.faces[2] = bodyTex.faces[3] = loadTexture("textures/snake_body_side.jpg");

    ResetGameWrapper();

    while (!glfwWindowShouldClose(window)) {
        float time = (float)glfwGetTime();
        deltaTime = time - lastFrame; lastFrame = time;
        camera.SetDeltaTime(deltaTime);
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        glClearColor(0.04f, 0.04f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)io.DisplaySize.x / (float)io.DisplaySize.y, 0.1f, 100.0f);
        glm::vec3 hPos = glm::vec3(snake[0].position) * (1.0f / 13.0f);
        glm::mat4 view = camera.GetViewMatrix(hPos, glm::vec3(snakeDir), glm::vec3(snakeUp));

        // --- OBSŁUGA STANÓW GRY (UŻYWA GameUI.h) ---
        if (currentState == GameState::Menu) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            // Wywołujemy menu, podając lambda funkcję "co zrobić jak klikniesz start"
            DrawMainMenu(currentState, [&]() { ResetGameWrapper(); });
        }
        else if (currentState == GameState::Settings) {
            // Przekazujemy zmienne, które menu może zmieniać, oraz co zrobić jak zmienisz liczbę jabłek
            DrawSettings(currentState, speedLevel, maxApples, camera.Mode, [&]() { spawnApplesAsNeeded(apples, maxApples, rng); });
        }

        else if (currentState == GameState::GameOver) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            DrawGameOver(currentState, finalScore, [&]() { ResetGameWrapper(); });
        }
        else if (currentState == GameState::Game) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) currentState = GameState::Menu;

            DrawHUD((int)snake.size() - 1); // Rysowanie licznika punktów
            // Przeliczanie poziomu na interwał czasowy
            if (speedLevel >= 10) {
                moveInterval = 0.001f; // Max prędkość (bardzo szybko)
            }
            else {
                // Poziom 1 = 0.9s (wolno), Poziom 5 = 0.5s, Poziom 9 = 0.1s (szybko)
                moveInterval = 1.0f - (speedLevel * 0.1f);
            }

            // ... dalej Twoja istniejąca logika: moveTimer += deltaTime; ...
            // LOGIKA RUCHU (Bez zmian, używa Logic.h)
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

            moveTimer += deltaTime;
            if (moveTimer >= moveInterval) {
                moveTimer = 0.0f; snakeDir = pendingDir; snakeUp = pendingUp;
                glm::ivec3 oldTail = snake.back().position;
                for (int i = (int)snake.size() - 1; i >= 1; --i) snake[i].position = snake[i - 1].position;
                snake[0].position += snakeDir;

                if (std::abs(snake[0].position.x) > 6 || std::abs(snake[0].position.y) > 6 || std::abs(snake[0].position.z) > 6) { finalScore = (int)snake.size() - 1; currentState = GameState::GameOver; }
                for (size_t i = 1; i < snake.size(); ++i) if (snake[i].position == snake[0].position) { finalScore = (int)snake.size() - 1; currentState = GameState::GameOver; }
                for (auto& app : apples) if (snake[0].position == app.position) { snake.push_back({ oldTail }); app.position = randomGridPos(rng); }
                canChangeDir = true;
            }

            drawBoundaryGrid(shader, view, projection);
            for (const auto& app : apples) drawApple(app.position, shader, VAO, appleTex, view, projection);
            for (size_t i = 0; i < snake.size(); ++i)
                drawSnakeSegment(snake[i].position, snakeDir, snakeUp, (i == 0), shader, VAO, headTex, bodyTex, view, projection);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}