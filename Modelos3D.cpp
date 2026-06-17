#include <glad/glad.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "Shader.h"
#include "Model.h"
#include "Player.h"
#include "LightSystem.h"
#include "Menu.h"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// ---- Estado global del juego ----
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Posición inicial del jugador (constante para restart)
static const glm::vec3 PLAYER_SPAWN(-56.0f, 10.0f, -86.807f);

Player player(PLAYER_SPAWN);
bool   inGame = false;   // true cuando el juego está activo
bool   gamePaused = false;

// ---- Callbacks ----
void framebuffer_size_callback(GLFWwindow* w, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (!inGame || gamePaused) return;
    float xpos = (float)xposIn, ypos = (float)yposIn;
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    player.camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (!inGame || gamePaused) return;
    player.camera.ProcessMouseScroll((float)yoffset);
}

// ---- Resetear jugador al inicio ----
void resetPlayer(Menu& menu) {
    player = Player(PLAYER_SPAWN);
    player.camera.MouseSensitivity = menu.mouseSensitivity;
    firstMouse = true;
    lastFrame = (float)glfwGetTime();
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        SCR_WIDTH, SCR_HEIGHT, "Hunted", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // ---- Shaders ----
    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    Shader lampShader("shaders/lamp.vert", "shaders/lamp.frag");

    // ---- Menú ----
    Menu menu(SCR_WIDTH, SCR_HEIGHT);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // ============================================================
    //  LOADING SCREEN INICIAL
    // ============================================================
    menu.state = MenuState::LOADING;
    Model* model = nullptr;
    float  fakeProgress = 0.0f;
    float  loadLastFrame = (float)glfwGetTime();
    bool   modelLoaded = false;

    while (!glfwWindowShouldClose(window) && !modelLoaded) {
        float now = (float)glfwGetTime();
        float dt = now - loadLastFrame;
        loadLastFrame = now;

        fakeProgress = glm::min(fakeProgress + dt * 0.30f, 0.9f);
        menu.SetLoadingProgress(fakeProgress);
        menu.blinkTimer += dt;
        menu.Update(window, dt);   // actualiza partículas / fade

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        menu.Render();
        glfwSwapBuffers(window);
        glfwPollEvents();

        if (fakeProgress >= 0.9f) {
            model = new Model("Resources/Models/sotanoCorregido.obj");
            modelLoaded = true;
            menu.SetLoadingProgress(1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            menu.Render();
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    // ============================================================
    //  MENÚ PRINCIPAL
    // ============================================================
    menu.state = MenuState::MAIN;
    menu.loadingProgress = 0.0f;
    float menuLastFrame = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window) &&
        menu.state != MenuState::PLAYING)
    {
        float now = (float)glfwGetTime();
        float dt = now - menuLastFrame;
        menuLastFrame = now;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        MenuAction action = menu.Update(window, dt);
        menu.Render();

        if (action == MenuAction::QUIT) {
            glfwSetWindowShouldClose(window, true); // Fuerza el cierre
            break;
        }
        if (menu.state == MenuState::LOADING) {
            menu.loadingProgress += dt * 0.8f;
            if (menu.loadingProgress >= 1.0f)
                menu.state = MenuState::PLAYING;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (glfwWindowShouldClose(window)) {
        delete model;
        glfwTerminate();
        return 0;
    }

    // ============================================================
    //  ENTRAR AL JUEGO
    // ============================================================
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    resetPlayer(menu);
    inGame = true;
    gamePaused = false;

    // ---- Sistema de luces ----
    LightSystem lightSystem;

    // ---- Cubo para lámparas ----
    float cubeVertices[] = {
        -0.5f,-0.05f,-0.2f,  0.5f,-0.05f,-0.2f,  0.5f, 0.05f,-0.2f,
         0.5f, 0.05f,-0.2f, -0.5f, 0.05f,-0.2f, -0.5f,-0.05f,-0.2f,
        -0.5f,-0.05f, 0.2f,  0.5f,-0.05f, 0.2f,  0.5f, 0.05f, 0.2f,
         0.5f, 0.05f, 0.2f, -0.5f, 0.05f, 0.2f, -0.5f,-0.05f, 0.2f,
        -0.5f, 0.05f, 0.2f, -0.5f, 0.05f,-0.2f, -0.5f,-0.05f,-0.2f,
        -0.5f,-0.05f,-0.2f, -0.5f,-0.05f, 0.2f, -0.5f, 0.05f, 0.2f,
         0.5f, 0.05f, 0.2f,  0.5f, 0.05f,-0.2f,  0.5f,-0.05f,-0.2f,
         0.5f,-0.05f,-0.2f,  0.5f,-0.05f, 0.2f,  0.5f, 0.05f, 0.2f,
        -0.5f,-0.05f,-0.2f,  0.5f,-0.05f,-0.2f,  0.5f,-0.05f, 0.2f,
         0.5f,-0.05f, 0.2f, -0.5f,-0.05f, 0.2f, -0.5f,-0.05f,-0.2f,
        -0.5f, 0.05f,-0.2f,  0.5f, 0.05f,-0.2f,  0.5f, 0.05f, 0.2f,
         0.5f, 0.05f, 0.2f, -0.5f, 0.05f, 0.2f, -0.5f, 0.05f,-0.2f,
    };
    unsigned int lampVAO, lampVBO;
    glGenVertexArrays(1, &lampVAO);
    glGenBuffers(1, &lampVBO);
    glBindVertexArray(lampVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lampVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    bool prevPauseKey = false;

    // ============================================================
    //  GAME LOOP
    // ============================================================
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // <--- CAMBIO AQUÍ: Ahora se usa la tecla P para abrir el menú
        bool pauseKeyNow = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;

        // ---- Toggle pausa ----
        if (pauseKeyNow && !prevPauseKey) {
            if (menu.state == MenuState::PLAYING) {
                menu.state = MenuState::PAUSED;
                gamePaused = true;
                menu.selectedItem = 0;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            // Si está en PAUSED, el Update del menú maneja la lógica
        }
        prevPauseKey = pauseKeyNow;

        MenuAction action = MenuAction::NONE;

        if (gamePaused) {
            // Actualizar y renderizar menú de pausa
            action = menu.Update(window, deltaTime);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Renderizar escena en background
            {
                glm::mat4 view = player.camera.GetViewMatrix();
                glm::mat4 projection = glm::perspective(
                    glm::radians(player.camera.Zoom),
                    (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 1000.0f);
                glm::mat4 modelMat = glm::mat4(1.0f);

                shader.use();
                glUniform1f(glGetUniformLocation(shader.ID, "brightness"),
                    menu.brightness);
                glUniform1i(glGetUniformLocation(shader.ID, "flashlightOn"),
                    player.flashlight.on);
                glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"),
                    1, GL_FALSE, glm::value_ptr(modelMat));
                glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"),
                    1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"),
                    1, GL_FALSE, glm::value_ptr(projection));
                model->Draw(shader.ID);
            }

            // Overlay del menú de pausa encima
            menu.Render();

        }
        else if (menu.state == MenuState::PLAYING) {
            player.ProcessInput(window, deltaTime);
            player.UpdatePhysics(deltaTime);
            player.UpdateFlashlight();

            glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 view = player.camera.GetViewMatrix();
            glm::mat4 projection = glm::perspective(
                glm::radians(player.camera.Zoom),
                (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 1000.0f);
            glm::mat4 modelMat = glm::mat4(1.0f);

            lightSystem.Update(deltaTime, currentFrame, player.camera.Position);

            shader.use();
            glUniform1i(glGetUniformLocation(shader.ID, "flashlightOn"),
                player.flashlight.on);
            glUniform3fv(glGetUniformLocation(shader.ID, "flashlightPos"),
                1, &player.flashlight.position[0]);
            glUniform3fv(glGetUniformLocation(shader.ID, "flashlightDir"),
                1, &player.flashlight.direction[0]);

            for (int i = 0; i < NUM_LAMPS; i++) {
                std::string base = "lights[" + std::to_string(i) + "]";
                float linear = 0.03f, quadratic = 0.007f;
                if (i == 2 || i == 3 || i == 4) { linear = 0.09f; quadratic = 0.032f; }

                glUniform3fv(glGetUniformLocation(shader.ID,
                    (base + ".position").c_str()),
                    1, &lightSystem.lampPositions[i][0]);
                glUniform3f(glGetUniformLocation(shader.ID,
                    (base + ".color").c_str()), 1.0f, 0.95f, 0.8f);
                glUniform1f(glGetUniformLocation(shader.ID,
                    (base + ".intensity").c_str()), lightSystem.intensities[i]);
                glUniform1f(glGetUniformLocation(shader.ID,
                    (base + ".constant").c_str()), 1.0f);
                glUniform1f(glGetUniformLocation(shader.ID,
                    (base + ".linear").c_str()), linear);
                glUniform1f(glGetUniformLocation(shader.ID,
                    (base + ".quadratic").c_str()), quadratic);
            }

            glUniform3fv(glGetUniformLocation(shader.ID, "viewPos"),
                1, &player.camera.Position[0]);
            glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"),
                1, GL_FALSE, glm::value_ptr(modelMat));
            glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"),
                1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"),
                1, GL_FALSE, glm::value_ptr(projection));
            model->Draw(shader.ID);

            // Lámparas
            lampShader.use();
            glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "view"),
                1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "projection"),
                1, GL_FALSE, glm::value_ptr(projection));
            glBindVertexArray(lampVAO);
            for (int i = 0; i < NUM_LAMPS; i++) {
                glm::mat4 lm = glm::translate(glm::mat4(1.0f),
                    lightSystem.lampPositions[i]);
                glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "model"),
                    1, GL_FALSE, glm::value_ptr(lm));
                float b = lightSystem.intensities[i];
                glUniform3f(glGetUniformLocation(lampShader.ID, "lampColor"),
                    b, 0.95f * b, 0.8f * b);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // ---- Procesar acciones del menú ----
        if (action == MenuAction::RESUME_GAME) {
            menu.state = MenuState::PLAYING;
            gamePaused = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
        }
        else if (action == MenuAction::RESTART_GAME) {
            resetPlayer(menu);
            menu.state = MenuState::PLAYING;
            menu.loadingProgress = 0.0f;
            gamePaused = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else if (action == MenuAction::GO_TO_MAIN) {
            menu.state = MenuState::MAIN;
            gamePaused = false;
            inGame = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            // Re-entrar al loop de menú principal
            menuLastFrame = (float)glfwGetTime();
            while (!glfwWindowShouldClose(window) &&
                menu.state != MenuState::PLAYING)
            {
                float n2 = (float)glfwGetTime();
                float d2 = n2 - menuLastFrame;
                menuLastFrame = n2;
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                MenuAction a2 = menu.Update(window, d2);
                menu.Render();
                if (a2 == MenuAction::QUIT) {
                    glfwSetWindowShouldClose(window, true);
                    goto cleanup;
                }
                if (menu.state == MenuState::LOADING) {
                    menu.loadingProgress += d2 * 0.8f;
                    if (menu.loadingProgress >= 1.0f)
                        menu.state = MenuState::PLAYING;
                }
                glfwSwapBuffers(window);
                glfwPollEvents();
            }
            if (menu.state == MenuState::PLAYING) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                resetPlayer(menu);
                inGame = true;
                gamePaused = false;
            }
        }
        else if (action == MenuAction::QUIT) {
            break;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

cleanup:
    delete model;
    glDeleteVertexArrays(1, &lampVAO);
    glDeleteBuffers(1, &lampVBO);
    glfwTerminate();
    return 0;
}
