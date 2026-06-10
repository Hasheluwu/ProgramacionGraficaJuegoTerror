#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <string>

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

// ==========================================
// CONFIGURACIÓN DE LA MATRIZ
// ==========================================
const float OFFSET_X = -76.40f;
const float OFFSET_Z = -24.00f;
const float TAMANO_BLOQUE = 0.8f;
const float CENTRO_BLOQUE = TAMANO_BLOQUE / 2.0f;

// 1. POSICIÓN INICIAL ACTUALIZADA
Player player(glm::vec3(129.0f, 5.0f, -98.0f));

std::vector<std::vector<int>> laberinto;

// --- FUNCIÓN VIEJA INTACTA PARA CARGAR EL TXT CON LLAVES {} ---
std::vector<std::vector<int>> cargarLaberinto(const std::string& ruta) {
    std::vector<std::vector<int>> matriz;
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        std::cout << "--> ERROR CRITICO: No se encontro el archivo " << ruta << std::endl;
        return matriz;
    }
    char c; int num; std::vector<int> filaActual;
    while (archivo >> c) {
        if (c == '{') filaActual.clear();
        else if (c == '}') { if (!filaActual.empty()) matriz.push_back(filaActual); }
        else if (isdigit(c)) {
            archivo.putback(c); archivo >> num; filaActual.push_back(num);
        }
        else if (c == ',') continue;
    }
    return matriz;
}

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

    // ---- Shaders ----
    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    Shader lampShader("shaders/lamp.vert", "shaders/lamp.frag");

    // ---- Modelos ----
    Model model("C:/Users/hashe/OneDrive/Escritorio/GraficaToday/Resources/Models/Casa/sotano2.obj");

    // Carga de la esfera para visualizar colisiones
    Model sphereModel("C:/Users/hashe/OneDrive/Escritorio/GraficaToday/Resources/Models/Casa/sphere/scene.gltf");

    // ---- Lector del archivo TXT de la Matriz ----
    std::cout << "[INFO] Leyendo matriz.txt..." << std::endl;
    laberinto = cargarLaberinto("C:/Users/hashe/OneDrive/Escritorio/GraficaToday/Resources/Models/Casa/matriz_tremen22.txt");
    if (laberinto.empty()) {
        std::cout << "ADVERTENCIA: La matriz esta vacia o no se pudo cargar." << std::endl;
    }
    else {
        std::cout << "Matriz cargada exitosamente." << std::endl;
    }

    // ---- Sistema de luces ----
    LightSystem lightSystem;

    // ---- Variables para el toggle de la letra P ----
    bool showCollisions = true;
    bool pKeyPressed = false;

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

        // ---- Lógica de la tecla P (Modo Debug de Colisiones) ----
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            if (!pKeyPressed) {
                showCollisions = !showCollisions; // Alternar estado
                pKeyPressed = true;
            }
        }
        else {
            pKeyPressed = false; // Reiniciar cuando se suelta la tecla
        }

        // GUARDAMOS LA POSICIÓN ANTES DE MOVER AL JUGADOR
        glm::vec3 oldPos = player.camera.Position;

        player.ProcessInput(window, deltaTime);
        player.UpdatePhysics(deltaTime);
        player.UpdateFlashlight();

        // =========================================================
        // ---- LÓGICA DE COLISIONES FÍSICAS ----
        // =========================================================
        if (!laberinto.empty()) {
            float margen = 0.15f;
            // Recordamos invertir el Z para la lectura matemática
            float blenderZ_cam = -player.camera.Position.z;

            int fMin = (int)floor(((blenderZ_cam - margen) - OFFSET_Z) / TAMANO_BLOQUE);
            int fMax = (int)floor(((blenderZ_cam + margen) - OFFSET_Z) / TAMANO_BLOQUE);
            int cIzq = (int)floor(((player.camera.Position.x - margen) - OFFSET_X) / TAMANO_BLOQUE);
            int cDer = (int)floor(((player.camera.Position.x + margen) - OFFSET_X) / TAMANO_BLOQUE);

            if (fMin < 0 || fMax >= (int)laberinto.size() || cIzq < 0 || cDer >= (int)laberinto[0].size() ||
                laberinto[fMin][cIzq] == 1 || laberinto[fMin][cDer] == 1 ||
                laberinto[fMax][cIzq] == 1 || laberinto[fMax][cDer] == 1) {

                // Colisión detectada, revertimos movimiento en X y Z
                player.camera.Position.x = oldPos.x;
                player.camera.Position.z = oldPos.z;
            }
        }

        glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = player.camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(player.camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.0f, 0.0f));

        // ---- Actualizar sistema de luces ----
        lightSystem.Update(deltaTime, currentFrame, player.camera.Position);

        // ---- Shader principal ----   
        shader.use();

        glUniform1i(glGetUniformLocation(shader.ID, "flashlightOn"), player.flashlight.on);
        glUniform3f(glGetUniformLocation(shader.ID, "flashlightPos"), player.flashlight.position.x, player.flashlight.position.y, player.flashlight.position.z);
        glUniform3f(glGetUniformLocation(shader.ID, "flashlightDir"), player.flashlight.direction.x, player.flashlight.direction.y, player.flashlight.direction.z);

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

            glUniform3f(glGetUniformLocation(shader.ID, (base + ".position").c_str()), lightSystem.lampPositions[i].x, lightSystem.lampPositions[i].y, lightSystem.lampPositions[i].z);
            glUniform3f(glGetUniformLocation(shader.ID, (base + ".color").c_str()), 1.0f, 0.95f, 0.8f);
            glUniform1f(glGetUniformLocation(shader.ID, (base + ".intensity").c_str()), lightSystem.intensities[i]);
            glUniform1f(glGetUniformLocation(shader.ID, (base + ".constant").c_str()), 1.0f);
            glUniform1f(glGetUniformLocation(shader.ID, (base + ".linear").c_str()), linear);
            glUniform1f(glGetUniformLocation(shader.ID, (base + ".quadratic").c_str()), quadratic);
        }

        glUniform3f(glGetUniformLocation(shader.ID, "viewPos"), player.camera.Position.x, player.camera.Position.y, player.camera.Position.z);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        model.Draw(shader.ID);

        // =======================================================================
        // ---- DIBUJAR ESFERAS DE COLISIÓN (Activado con la tecla 'P') ----
        // =======================================================================
        if (showCollisions && !laberinto.empty()) {
            for (int f = 0; f < laberinto.size(); f++) {
                for (int c = 0; c < laberinto[0].size(); c++) {

                    float posX = OFFSET_X + (c * TAMANO_BLOQUE) + CENTRO_BLOQUE;
                    float posZ = -(OFFSET_Z + (f * TAMANO_BLOQUE) + CENTRO_BLOQUE);

                    float distanciaAlJugador = sqrt(pow(player.camera.Position.x - posX, 2) + pow(player.camera.Position.z - posZ, 2));

                    if (distanciaAlJugador < 12.0f) {
                        if (laberinto[f][c] == 1) {
                            glm::mat4 debugMat = glm::mat4(1.0f);

                            // Ajustado a 5.0f exactos para que esté a la misma altura que tú
                            float alturaPecho = 5.0f;

                            debugMat = glm::translate(debugMat, glm::vec3(posX, alturaPecho, posZ));
                            debugMat = glm::scale(debugMat, glm::vec3(0.08f));

                            glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(debugMat));

                            // Color verde para distinguirlas
                            glUniform3f(glGetUniformLocation(shader.ID, "colorTint"), 0.0f, 1.0f, 0.0f);
                            sphereModel.Draw(shader.ID);
                        }
                    }
                }
            }
            // Restaurar el tinte a blanco
            glUniform3f(glGetUniformLocation(shader.ID, "colorTint"), 1.0f, 1.0f, 1.0f);
        }

        // ---- Dibujar cubos de lampara ----
        lampShader.use();
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(lampVAO);
        for (int i = 0; i < NUM_LAMPS; i++)
        {
            glm::mat4 lampMat = glm::mat4(1.0f);
            lampMat = glm::translate(lampMat, lightSystem.lampPositions[i]);
            glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lampMat));

            float bright = lightSystem.intensities[i];
            glUniform3f(glGetUniformLocation(lampShader.ID, "lampColor"), 1.0f * bright, 0.95f * bright, 0.8f * bright);

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