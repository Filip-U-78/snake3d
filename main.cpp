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

// --- DODANO STB_IMAGE ---
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Upewnij się, że ścieżka jest poprawna

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "include/Shader.h"
#include "include/OrbitCamera.h"
#include <filesystem>

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
struct UV {
    float u0, v0;
    float u1, v1;
};

UV atlasUV(int col, int row, int cols, int rows) {
    float w = 1.0f / cols;
    float h = 1.0f / rows;

    return {
        col * w,
        1.0f - (row + 1) * h,
        (col + 1) * w,
        1.0f - row * h
    };
}


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
int finalScore = 0;

std::vector<Apple> apples;
int maxApples = 3;

std::random_device rd;
std::mt19937 rng(rd());

OrbitCamera camera(3.0f, glm::vec3(0.0f, 0.0f, 0.0f));

// --- FUNKCJA WCZYTUJĄCA TEKSTURY ---
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true); // Odwracamy oś Y obrazka dla OpenGL
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;


        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Powtarzanie tekstury i filtrowanie
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

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
    camera.ProcessMouseMovement((float)xpos - lastX, (float)lastY - (float)ypos);
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
    Shader& shader, unsigned int VAO, unsigned int snakeTex, const glm::mat4& view, const glm::mat4& projection) {

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos) * (1.0f / 13.0f));
    model *= rotationFromForwardUp(f, u);
    model = glm::scale(model, glm::vec3(1.0f / 13.0f));

    glm::mat4 mvp = projection * view * model;
    shader.setMat4("MVP", mvp);

    // Włączamy teksturowanie
    shader.setBool("useTexture", true);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, snakeTex);

    glBindVertexArray(VAO);
    if (isHead) {
        // Głowę barwimy lekko na niebiesko, ale używając tekstury
        shader.setVec4("color", glm::vec4(0.8f, 0.8f, 1.0f, 1.0f));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    else {
        // Ciało: biały kolor = oryginalny kolor tekstury
        shader.setVec4("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
}

void drawApple(const glm::ivec3& pos, Shader& shader, unsigned int VAO, unsigned int appleTex, const glm::mat4& view, const glm::mat4& projection) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos) * (1.0f / 13.0f));
    model = glm::scale(model, glm::vec3(1.0f / 13.0f));

    glm::mat4 mvp = projection * view * model;
    shader.setMat4("MVP", mvp);

    shader.setBool("useTexture", true);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, appleTex);

    // Jabłko bez tintu (oryginalna tekstura)
    shader.setVec4("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawBoundaryGrid(Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    constexpr float B = 6.0f;
    constexpr float S = 1.0f / 13.0f;

    glm::vec3 edges[] = {
        {-B,-B,-B},{ B,-B,-B}, { B,-B,-B},{ B, B,-B}, { B, B,-B},{-B, B,-B}, {-B, B,-B},{-B,-B,-B},
        {-B,-B, B},{ B,-B, B}, { B,-B, B},{ B, B, B}, { B, B, B},{-B, B, B}, {-B, B, B},{-B,-B, B},
        {-B,-B,-B},{-B,-B, B}, { B,-B,-B},{ B,-B, B},
        { B, B,-B},{ B, B, B}, {-B, B,-B},{-B, B, B}
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(edges), edges, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    // Tutaj NIE włączamy atrybutu 1 (tekstur), bo siatka ich nie ma

    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(S));
    glm::mat4 mvp = projection * view * model;

    shader.use();
    shader.setMat4("MVP", mvp);
    shader.setBool("useTexture", false); // Wyłącz tekstury dla siatki
    shader.setVec4("color", glm::vec4(0.6f, 0.8f, 1.0f, 0.25f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.5f);

    glDrawArrays(GL_LINES, 0, 24);

    glLineWidth(1.0f);
    glDisable(GL_BLEND);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1000, 800, "Snake 3D Textures", NULL, NULL);
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
    shader.use();
    shader.setInt("ourTexture", 0);


    // NOWA TABLICA WIERZCHOŁKÓW: [x,y,z, u,v]
    float cubeVertices[] = {
        // positions          // texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Atr 0: Pozycja (3 floaty)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Atr 1: Tekstura UV (2 floaty) - offset to 3 * float
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // WCZYTYWANIE TEKSTUR
    unsigned int snakeTexture = loadTexture("snake.jpg");
    unsigned int appleTexture = loadTexture("apple.jpg");

    shader.use();
    shader.setInt("ourTexture", 0); // Ustawiamy sampler na slot 0

    resetGame();
    const char* camModes[] = { "Orbit (myszka)", "Pierwsza osoba (FPP)", "Podążająca (TPP)" };

    while (!glfwWindowShouldClose(window)) {
        float time = (float)glfwGetTime();
        deltaTime = time - lastFrame; lastFrame = time;
        camera.SetDeltaTime(deltaTime);
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glClearColor(0.04f, 0.04f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)io.DisplaySize.x / (float)io.DisplaySize.y, 0.1f, 100.0f);

        glm::vec3 headPos = glm::vec3(snake[0].position) * (1.0f / 13.0f);
        glm::vec3 headDir = glm::vec3(snakeDir);
        glm::vec3 headUp = glm::vec3(snakeUp);
        glm::mat4 view = camera.GetViewMatrix(headPos, headDir, headUp);

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
            int modeInt = static_cast<int>(camera.Mode);
            if (ImGui::Combo("Tryb kamery", &modeInt, camModes, IM_ARRAYSIZE(camModes))) {
                camera.SetMode(static_cast<CameraMode>(modeInt));
            }
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
            if (ImGui::Button("ZAGRAJ PONOWNIE", ImVec2(220, 45))) { resetGame(); currentState = GameState::Game; }
            if (ImGui::Button("MENU GLOWNE", ImVec2(220, 45))) currentState = GameState::Menu;
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
                moveTimer = 0.0f;
                snakeDir = pendingDir; snakeUp = pendingUp;
                glm::ivec3 oldTail = snake.back().position;
                for (int i = (int)snake.size() - 1; i >= 1; --i) snake[i].position = snake[i - 1].position;
                snake[0].position += snakeDir;

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

            // Renderowanie
            shader.use();
            glDepthMask(GL_FALSE);
            drawBoundaryGrid(shader, view, projection);
            glDepthMask(GL_TRUE);

            for (const auto& app : apples)
                drawApple(app.position, shader, VAO, appleTexture, view, projection);

            for (size_t i = 0; i < snake.size(); ++i)
                drawSnakeSegment(snake[i].position, snakeDir, snakeUp, (i == 0), shader, VAO, snakeTexture, view, projection);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}