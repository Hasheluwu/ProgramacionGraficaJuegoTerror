#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "Shader.h"
#include "Model.h"
#include "Camera.h"

// ---------------- WINDOW ----------------

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// ---------------- CAMERA ----------------

Camera camera(glm::vec3(0.0f, 2.0f, 8.0f));

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ---------------- CALLBACKS ----------------

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = (float)xposIn;
    float ypos = (float)yposIn;

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll((float)yoffset);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// ---------------- MAIN ----------------

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Modelo 3D + Luz", NULL, NULL);
    if (!window)
    {
        std::cout << "ERROR WINDOW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "ERROR GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // ---------------- SHADER ----------------

    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");

    // ---------------- MODEL ----------------

    Model model("C:/Users/m1xim/source/repos/Modelos3D/Resources/Models/Sahur/car.obj");

    float angle = 0.0f;

    // ---------------- LOOP ----------------

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---------------- LUZ ORBITAL ----------------
        angle += deltaTime;

        glm::vec3 lightPos;
        lightPos.x = sin(angle) * 5.0f;
        lightPos.y = 2.0f;
        lightPos.z = cos(angle) * 5.0f;

        // ---------------- SHADER ----------------
        shader.use();

        glUniform3f(
            glGetUniformLocation(shader.ID, "lightPos"),
            lightPos.x, lightPos.y, lightPos.z
        );

        glUniform3f(
            glGetUniformLocation(shader.ID, "viewPos"),
            camera.Position.x,
            camera.Position.y,
            camera.Position.z
        );

        // ---------------- MATRICES ----------------

        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(1.0f));

        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f,
            1000.0f
        );

        glUniformMatrix4fv(
            glGetUniformLocation(shader.ID, "model"),
            1,
            GL_FALSE,
            glm::value_ptr(modelMat)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(shader.ID, "view"),
            1,
            GL_FALSE,
            glm::value_ptr(view)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(shader.ID, "projection"),
            1,
            GL_FALSE,
            glm::value_ptr(projection)
        );

        // ---------------- DRAW ----------------
        model.Draw(shader.ID);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}