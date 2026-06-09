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

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Player player(glm::vec3(129.0f, 5.0f, -98.0f));

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

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Sotano", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // <-- agregado

    // ---- Shaders ----
    Shader shader("C:/Users/garci/source/repos/Project/iluminacion/vertex.glsl",
        "C:/Users/garci/source/repos/Project/iluminacion/fragment.glsl");
    Shader lampShader("C:/Users/garci/source/repos/Project/iluminacion/lamp.vert",
        "C:/Users/garci/source/repos/Project/iluminacion/lamp.frag");

    // ---- Modelo ----
    Model model("res/Models/sotanoCorregido.obj");

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
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 1000.0f); // <-- 0.01f
        glm::mat4 modelMat = glm::mat4(1.0f);
        // modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.0f, 0.0f));

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

        model.Draw(shader.ID);

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

    glDeleteVertexArrays(1, &lampVAO);
    glDeleteBuffers(1, &lampVBO);
    glfwTerminate();
    return 0;
}