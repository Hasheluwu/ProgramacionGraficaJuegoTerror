#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include "Shader.h"
#include "Model.h"
#include "Player.h"
#include "LightSystem.h"
#include "Menu.h"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Player player(glm::vec3(-56.0f, 10.0f, -86.807f));

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = (float)xposIn, ypos = (float)yposIn;
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    player.camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    player.camera.ProcessMouseScroll((float)yoffset);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hunted", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // ---- Shaders ----
    Shader shader(
        "shaders/vertex.glsl",
        "shaders/fragment.glsl");
    Shader lampShader(
        "shaders/lamp.vert",
        "shaders/lamp.frag");

    // ---- Menu ----
    Menu menu(SCR_WIDTH, SCR_HEIGHT);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // ---- Pantalla de carga inicial ----
    menu.state = MenuState::LOADING;
    Model* model = nullptr;

    float fakeProgress = 0.0f;
    float loadLastFrame = (float)glfwGetTime();
    bool modelLoaded = false;

    while (!glfwWindowShouldClose(window) && !modelLoaded)
    {
        float now = (float)glfwGetTime();
        float dt = now - loadLastFrame;
        loadLastFrame = now;

        if (fakeProgress < 0.9f)
        {
            fakeProgress += dt * 0.30f;

            if (fakeProgress > 0.9f)
                fakeProgress = 0.9f;
        }

        menu.SetLoadingProgress(fakeProgress);
        menu.blinkTimer += dt;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        menu.Render();

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (fakeProgress >= 0.9f)
        {
            model = new Model("Resources/Models/sotanoCorregido.obj");

            modelLoaded = true;

            menu.SetLoadingProgress(1.0f);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            menu.Render();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    // ---- Menu principal ----
    menu.state = MenuState::MAIN;
    menu.loadingProgress = 0.0f;
    float menuLastFrame = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window) && menu.state != MenuState::PLAYING)
    {
        float now = (float)glfwGetTime();
        float dt = now - menuLastFrame;
        menuLastFrame = now;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (menu.state == MenuState::SETTINGS)
            menu.renderSettings(window);
        else if (menu.state == MenuState::LOADING)
        {
            menu.loadingProgress += dt * 0.8f;
            menu.blinkTimer += dt;
            if (menu.loadingProgress >= 1.0f)
                menu.state = MenuState::PLAYING;
            menu.Render();
        }
        else
            menu.Render();

        menu.Update(window, dt);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (glfwWindowShouldClose(window))
    {
        delete model;
        glfwTerminate();
        return 0;
    }

    // ---- Al entrar al juego ocultar cursor ----
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    firstMouse = true;

    // Aplicar sensibilidad del menu al jugador
    player.camera.MouseSensitivity = menu.mouseSensitivity;

    // ---- Sistema de luces ----
    LightSystem lightSystem;

    // ---- Cubo para las lamparas ----
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

    // ---- Game loop ----
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        player.ProcessInput(window, deltaTime);
        player.UpdatePhysics(deltaTime);
        player.UpdateFlashlight();

        glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = player.camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(player.camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 1000.0f);
        glm::mat4 modelMat = glm::mat4(1.0f);

        lightSystem.Update(deltaTime, currentFrame, player.camera.Position);

        shader.use();

        glUniform1i(glGetUniformLocation(shader.ID, "flashlightOn"),
            player.flashlight.on);
        glUniform3f(glGetUniformLocation(shader.ID, "flashlightPos"),
            player.flashlight.position.x,
            player.flashlight.position.y,
            player.flashlight.position.z);
        glUniform3f(glGetUniformLocation(shader.ID, "flashlightDir"),
            player.flashlight.direction.x,
            player.flashlight.direction.y,
            player.flashlight.direction.z);

        for (int i = 0; i < NUM_LAMPS; i++)
        {
            std::string base = "lights[" + std::to_string(i) + "]";

            float linear = 0.03f;
            float quadratic = 0.007f;
            if (i == 2 || i == 3 || i == 4)
            {
                linear = 0.09f;
                quadratic = 0.032f;
            }

            glUniform3f(glGetUniformLocation(shader.ID, (base + ".position").c_str()),
                lightSystem.lampPositions[i].x,
                lightSystem.lampPositions[i].y,
                lightSystem.lampPositions[i].z);
            glUniform3f(glGetUniformLocation(shader.ID, (base + ".color").c_str()),
                1.0f, 0.95f, 0.8f);
            glUniform1f(glGetUniformLocation(shader.ID, (base + ".intensity").c_str()),
                lightSystem.intensities[i]);
            glUniform1f(glGetUniformLocation(shader.ID, (base + ".constant").c_str()), 1.0f);
            glUniform1f(glGetUniformLocation(shader.ID, (base + ".linear").c_str()), linear);
            glUniform1f(glGetUniformLocation(shader.ID, (base + ".quadratic").c_str()), quadratic);
        }

        glUniform3f(glGetUniformLocation(shader.ID, "viewPos"),
            player.camera.Position.x, player.camera.Position.y, player.camera.Position.z);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"),
            1, GL_FALSE, glm::value_ptr(modelMat));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"),
            1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"),
            1, GL_FALSE, glm::value_ptr(projection));

        model->Draw(shader.ID);

        lampShader.use();
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "view"),
            1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "projection"),
            1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(lampVAO);
        for (int i = 0; i < NUM_LAMPS; i++)
        {
            glm::mat4 lampMat = glm::mat4(1.0f);
            lampMat = glm::translate(lampMat, lightSystem.lampPositions[i]);
            glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "model"),
                1, GL_FALSE, glm::value_ptr(lampMat));

            float bright = lightSystem.intensities[i];
            glUniform3f(glGetUniformLocation(lampShader.ID, "lampColor"),
                1.0f * bright, 0.95f * bright, 0.8f * bright);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete model;
    glDeleteVertexArrays(1, &lampVAO);
    glDeleteBuffers(1, &lampVBO);
    glfwTerminate();
    return 0;
}
