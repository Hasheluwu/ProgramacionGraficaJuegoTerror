#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>

#include "Shader.h"
#include "Model.h"
#include "Camera.h"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// ==========================================
// CONFIGURACIÓN DE LA MATRIZ Y EL MAPA NUEVO
// ==========================================
const float OFFSET_X = -76.40f;
const float OFFSET_Z = -24.00f;
const float TAMANO_BLOQUE = 0.8f;
const float CENTRO_BLOQUE = TAMANO_BLOQUE / 2.0f;
float NIVEL_DEL_SUELO = 2.27f;

// ==========================================
// SPAWN DINÁMICO (Al no haber waypoints)
// ==========================================
// Te colocamos 5 metros adentro de la esquina superior izquierda de la matriz
float SPAWN_X = OFFSET_X + 5.0f;
float SPAWN_Y = NIVEL_DEL_SUELO; // Tu altura base
float SPAWN_Z = -(OFFSET_Z + 5.0f); // Respetando el efecto espejo de Blender a OpenGL

Camera camera(glm::vec3(SPAWN_X, SPAWN_Y, SPAWN_Z), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float debugTimer = 0.0f;

std::vector<std::vector<int>> laberinto;

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = (float)xposIn; float ypos = (float)yposIn;
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    camera.ProcessMouseMovement(xpos - lastX, lastY - ypos);
    lastX = xpos; lastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) { camera.ProcessMouseScroll((float)yoffset); }

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

    glm::vec3 oldPos = camera.Position;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.ProcessKeyboard(UPWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.ProcessKeyboard(DOWNWARD, deltaTime);

    if (camera.Position.y < (NIVEL_DEL_SUELO + 3.0f) && !laberinto.empty()) {
        float margen = 0.15f;
        float blenderZ_cam = -camera.Position.z;

        int fMin = (int)floor(((blenderZ_cam - margen) - OFFSET_Z) / TAMANO_BLOQUE);
        int fMax = (int)floor(((blenderZ_cam + margen) - OFFSET_Z) / TAMANO_BLOQUE);
        int cIzq = (int)floor(((camera.Position.x - margen) - OFFSET_X) / TAMANO_BLOQUE);
        int cDer = (int)floor(((camera.Position.x + margen) - OFFSET_X) / TAMANO_BLOQUE);

        // APLICADO EL BLINDAJE (int) A LOS .size()
        if (fMin < 0 || fMax >= (int)laberinto.size() || cIzq < 0 || cDer >= (int)laberinto[0].size() ||
            laberinto[fMin][cIzq] == 1 || laberinto[fMin][cDer] == 1 ||
            laberinto[fMax][cIzq] == 1 || laberinto[fMax][cDer] == 1) {

            camera.Position.x = oldPos.x;
            camera.Position.z = oldPos.z;
        }
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Auditor de Colisiones - Solo Camara", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");

    std::cout << "[INFO] Leyendo matriz.txt..." << std::endl;
    // Corregido el "uwu" por si acaso ese era el archivo real
    laberinto = cargarLaberinto("Resources/matriz_sotanocorregidoend.txt");
    if (laberinto.empty()) return -1;

    // ==========================================
    // DEBUGGING DE MODELOS Y TEXTURAS
    // ==========================================
    std::cout << "[DEBUG] Intentando cargar el modelo 3D del sotano..." << std::endl;
    Model sotanoModel("Resources/sotano.obj");
    std::cout << "[DEBUG] EXITO: Modelo del sotano cargado correctamente en memoria." << std::endl;

    std::cout << "[DEBUG] Intentando cargar el modelo de la esfera..." << std::endl;
    Model esferaModel("Resources/sphere/scene.gltf");
    std::cout << "[DEBUG] EXITO: Modelo de la esfera cargado correctamente en memoria." << std::endl;
    // ==========================================

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        debugTimer += deltaTime;
        if (debugTimer >= 1.0f) {
            int cX = (int)floor((camera.Position.x - OFFSET_X) / TAMANO_BLOQUE);
            int cZ = (int)floor((-camera.Position.z - OFFSET_Z) / TAMANO_BLOQUE);

            std::cout << "\n--- ESTADO DEL JUGADOR ---" << std::endl;
            std::cout << "Mundo 3D -> X: " << camera.Position.x << " | Z: " << camera.Position.z << std::endl;
            if (cZ >= 0 && cZ < (int)laberinto.size() && cX >= 0 && cX < (int)laberinto[0].size()) {
                std::cout << "Casilla actual: [" << laberinto[cZ][cX] << "]" << std::endl;
            }
            debugTimer = 0.0f;
        }

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        glUniform3f(glGetUniformLocation(shader.ID, "lightPos"), 50.0f, 50.0f, 50.0f);
        glUniform3f(glGetUniformLocation(shader.ID, "viewPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(camera.GetViewMatrix()));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f)));

        glUniform3f(glGetUniformLocation(shader.ID, "colorTint"), 1.0f, 1.0f, 1.0f);

        glm::mat4 sotanoMat = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(sotanoMat));
        sotanoModel.Draw(shader.ID);

        // --- DEBUGGER VISUAL DE COLISIONES (Bolas Verdes a la altura del pecho) ---
        for (int f = 0; f < laberinto.size(); f++) {
            for (int c = 0; c < laberinto[0].size(); c++) {

                float posX = OFFSET_X + (c * TAMANO_BLOQUE) + CENTRO_BLOQUE;
                float posZ = -(OFFSET_Z + (f * TAMANO_BLOQUE) + CENTRO_BLOQUE);

                float distanciaAlJugador = sqrt(pow(camera.Position.x - posX, 2) + pow(camera.Position.z - posZ, 2));

                if (distanciaAlJugador < 12.0f) {
                    if (laberinto[f][c] == 1) {
                        glm::mat4 debugMat = glm::mat4(1.0f);

                        // ALTURA FIJA: 0.4 unidades debajo de tus "ojos" (cámara), lo que simula tu pecho.
                        float alturaPecho = SPAWN_Y - 0.4f;

                        debugMat = glm::translate(debugMat, glm::vec3(posX, alturaPecho, posZ));
                        debugMat = glm::scale(debugMat, glm::vec3(0.08f));

                        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(debugMat));

                        glUniform3f(glGetUniformLocation(shader.ID, "colorTint"), 0.0f, 1.0f, 0.0f);
                        esferaModel.Draw(shader.ID);
                    }
                }
            }
        }

        glUniform3f(glGetUniformLocation(shader.ID, "colorTint"), 1.0f, 1.0f, 1.0f);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}