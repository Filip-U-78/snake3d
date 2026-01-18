#include <iostream>
#include "include/Shader.h"
#include "include/OrbitCamera.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>




struct SnakeSegment {
    glm::ivec3 position; // pozycja w gridzie
};

std::vector<SnakeSegment> snake(1); // wąż na razie 1 segment
glm::ivec3 snakeDir(1, 0, 0);       // początkowy kierunek
glm::ivec3 pendingDir = snakeDir;
bool canChangeDir = true;
float moveTimer = 0.0f;
float moveInterval = 1.0f;          // czas między skokami w jednostkach siatki




bool mouseEnabled = true; // czy obracanie myszką jest aktywne

float deltaTime = 0.0f;
float lastFrame = 0.0f;



float cubeVertices[] = {
    // pozycje wierzcho?ków (x, y, z)
    // front face
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    // back face
    -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    // left face
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    // right face
     0.5f,  0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     // top face
     -0.5f,  0.5f,  0.5f,
      0.5f,  0.5f,  0.5f,
      0.5f,  0.5f, -0.5f,
      0.5f,  0.5f, -0.5f,
     -0.5f,  0.5f, -0.5f,
     -0.5f,  0.5f,  0.5f,
     // bottom face
     -0.5f, -0.5f,  0.5f,
     -0.5f, -0.5f, -0.5f,
      0.5f, -0.5f, -0.5f,
      0.5f, -0.5f, -0.5f,
      0.5f, -0.5f,  0.5f,
     -0.5f, -0.5f,  0.5f
};



OrbitCamera camera(3.0f, glm::vec3(0.0f, 0.0f, 0.0f));
float lastX = 400, lastY = 300;
bool firstMouse = true;



void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!mouseEnabled) return; // ignorujemy ruchy myszy, jeśli tryb wyłączony

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // odwrócone Y
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

glm::mat4 directionRotation(const glm::ivec3& dir)
{
    glm::mat4 rot = glm::mat4(1.0f);

    // =========================
    // RUCH WZDŁUŻ X (±1, 0, 0)
    // Niebieska ściana -> +Y
    // =========================
    if (dir.x != 0)
    {
        // +Z -> +Y  => obrót -90° wokół osi X
        rot = glm::rotate(rot, glm::radians(-90.0f), glm::vec3(1, 0, 0));
    }

    // =========================
    // RUCH WZDŁUŻ Y (0, ±1, 0)
    // Niebieska ściana -> -Z (do kamery)
    // =========================
    else if (dir.y != 0)
    {
        // +Z -> -Z  => obrót 180° wokół osi Y
        rot = glm::rotate(rot, glm::radians(180.0f), glm::vec3(0, 1, 0));
    }

    // =========================
    // RUCH WZDŁUŻ Z (0, 0, +1)
    // Niebieska ściana -> +Y
    // =========================
    else if (dir.z == 1)
    {
        // +Z -> +Y => obrót -90° wokół osi X
        rot = glm::rotate(rot, glm::radians(-90.0f), glm::vec3(1, 0, 0));
    }

    // =========================
    // RUCH WZDŁUŻ Z (0, 0, -1)
    // (opcjonalnie – symetria)
    // =========================
    else if (dir.z == -1)
    {
        // +Z -> -Y
        rot = glm::rotate(rot, glm::radians(-90.0f), glm::vec3(1, 0, 0));
    }

    return rot;
}


void drawSnakeSegment(const glm::ivec3& pos, const glm::ivec3& dir, Shader& shader, unsigned int cubeVAO, const glm::mat4& view, const glm::mat4& projection) {
    glm::vec3 snakeWorldPos = glm::vec3(pos) * (1.0f / 13.0f);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), snakeWorldPos);
    model *= directionRotation(dir);
    model = glm::scale(model, glm::vec3(1.0f / 13.0f));

    glm::mat4 MVP_snake = projection * view * model;
    unsigned int mvpLoc = glGetUniformLocation(shader.ID, "MVP");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP_snake));

    glBindVertexArray(cubeVAO);

    // niebieska ściana
    shader.setVec4("color", glm::vec4(0, 0, 1, 1));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // pozostałe ściany czerwone
    shader.setVec4("color", glm::vec4(1, 0, 0, 1));
    glDrawArrays(GL_TRIANGLES, 6, 30);
}



int main() {

    if (!glfwInit()) {
        std::cout << "GLFW init failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Snake 3D", nullptr, nullptr);
    if (!window) {
        std::cout << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "GLAD failed\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader shader("shaders/basic.vert", "shaders/basic.frag");



    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    snake[0].position = glm::ivec3(0, 0, 0);



    while (!glfwWindowShouldClose(window)) {



        std::cout << "\rSnakeDir = ("
            << snakeDir.x << ", "
            << snakeDir.y << ", "
            << snakeDir.z << ")   " << std::flush;





        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;




        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            mouseEnabled = !mouseEnabled;
            if (mouseEnabled) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                firstMouse = true; // resetujemy offsety, żeby uniknąć skoku przy ponownym włączeniu
            }
        }



        glm::ivec3 newDir = snakeDir;

        if (canChangeDir) {

            // =====================
            // RUCH WZDŁUŻ X
            // =====================
            if (snakeDir.x != 0) {
                int sign = snakeDir.x;

                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                    newDir = glm::ivec3(0, 1, 0);
                else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                    newDir = glm::ivec3(0, -1, 0);
                else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                    newDir = (sign > 0) ? glm::ivec3(0, 0, -1) : glm::ivec3(0, 0, 1);
                else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                    newDir = (sign > 0) ? glm::ivec3(0, 0, 1) : glm::ivec3(0, 0, -1);
            }

            // =====================
            // RUCH WZDŁUŻ Y
            // =====================
            else if (snakeDir.y != 0) {
                int sign = snakeDir.y;

                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                    newDir = glm::ivec3(0, 0, -1);
                else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                    newDir = glm::ivec3(0, 0, 1);
                else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                    newDir = (sign > 0) ? glm::ivec3(1, 0, 0) : glm::ivec3(-1, 0, 0);
                else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                    newDir = (sign > 0) ? glm::ivec3(-1, 0, 0) : glm::ivec3(1, 0, 0);
            }

            // =====================
            // RUCH WZDŁUŻ Z
            // =====================
            else if (snakeDir.z != 0) {
                int sign = snakeDir.z;

                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                    newDir = glm::ivec3(0, 1, 0);
                else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                    newDir = glm::ivec3(0, -1, 0);
                else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                    newDir = (sign > 0) ? glm::ivec3(1, 0, 0) : glm::ivec3(-1, 0, 0);
                else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                    newDir = (sign > 0) ? glm::ivec3(-1, 0, 0) : glm::ivec3(1, 0, 0);
            }

            // =====================
            // ZAKAZ COFANIA
            // =====================
            if (newDir != snakeDir && newDir != -snakeDir) {
                pendingDir = newDir;
                canChangeDir = false;   // 🔒 blokada do następnego ruchu
            }
        }


        



        moveTimer += deltaTime;
        if (moveTimer >= moveInterval) {
            moveTimer = 0.0f;

            snakeDir = pendingDir;          // ← zatwierdzenie decyzji
            snake[0].position += snakeDir; // ← ruch w siatce

            canChangeDir = true;            // ← odblokowanie sterowania
        }




        float limit = 6.0f;

        SnakeSegment& head = snake[0];
        if (head.position.x > 6 || head.position.x < -6 ||
            head.position.y > 6 || head.position.y < -6 ||
            head.position.z > 6 || head.position.z < -6)
        {
            head.position = glm::ivec3(0, 0, 0);

            snakeDir = glm::ivec3(1, 0, 0);   // kierunek startowy
            pendingDir = snakeDir;              // ❗ KLUCZOWE
            canChangeDir = true;                // odblokuj sterowanie

            moveTimer = 0.0f;                   // opcjonalnie: synchronizacja ticka

        }




        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        shader.use();

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        glm::mat4 MVP = projection * view * glm::mat4(1.0f);


        //glm::mat4 MVP = projection * view * model;
        unsigned int mvpLoc = glGetUniformLocation(shader.ID, "MVP");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        // Rysowanie szescianu
        glBindVertexArray(cubeVAO);
        shader.setVec4("color", glm::vec4(0.0f, 1.0f, 0.0f, 0.2f)); // zielony, alfa=0.2
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        drawSnakeSegment(snake[0].position, snakeDir, shader, cubeVAO, view, projection);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glfwTerminate();
    return 0;
}
