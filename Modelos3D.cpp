#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SOIL2/SOIL2.h> 
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <random>
#include "Shader.h"
#include "Model.h"
#include "Camera.h"
#include "IA/Pathfinding.h" 

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

float ALTURA_JUGADOR = 2.0f;
float ALTURA_ESFERA = 1.0f;

// ==========================================
// CONFIGURACIÓN DE PUNTO CERO Y ALTURA
// ==========================================
const float OFFSET_X = -68.38f;
const float OFFSET_Z = -1.67f;

float NIVEL_DEL_SUELO = 3.8f;

// ==========================================
// ¡AQUÍ PONDRÁS LAS COORDENADAS EXACTAS DE TU SPAWN!
// ==========================================
float SPAWN_X = -65.0f;
float SPAWN_Y = NIVEL_DEL_SUELO + 2.0f;
float SPAWN_Z = -10.0f; // Punto seguro para no caer fuera de la matriz

const float TAMANO_BLOQUE = 0.8f;
const float CENTRO_BLOQUE = TAMANO_BLOQUE / 2.0f;

Camera camera(glm::vec3(0.0f, ALTURA_JUGADOR, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

float lastX = SCR_WIDTH / 2.0f, lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;
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

std::vector<glm::ivec2> waypoints;

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

    // SISTEMA DE COLISIONES INVERTIDO
    if (camera.Position.y < (NIVEL_DEL_SUELO + 3.0f) && laberinto.size() > 0) {
        float margen = 0.15f;

        float blenderZ_cam = -camera.Position.z;

        int fMin = (int)floor(((blenderZ_cam - margen) - OFFSET_Z) / TAMANO_BLOQUE);
        int fMax = (int)floor(((blenderZ_cam + margen) - OFFSET_Z) / TAMANO_BLOQUE);
        int cIzq = (int)floor(((camera.Position.x - margen) - OFFSET_X) / TAMANO_BLOQUE);
        int cDer = (int)floor(((camera.Position.x + margen) - OFFSET_X) / TAMANO_BLOQUE);

        // Si chocamos contra una pared o nos salimos del mapa...
        if (fMin < 0 || fMax >= laberinto.size() || cIzq < 0 || cDer >= laberinto[0].size() ||
            laberinto[fMin][cIzq] == 1 || laberinto[fMin][cDer] == 1 ||
            laberinto[fMax][cIzq] == 1 || laberinto[fMax][cDer] == 1) {

            // ¡EL ARREGLO! Solo bloqueamos los ejes X y Z. 
            // Si te trabas, presiona la flecha Arriba para escapar.
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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Motor Grafico - Debugger de Spawn", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");

    std::cout << "[INFO] Leyendo matriz_definit.txt..." << std::endl;
    laberinto = cargarLaberinto("Resources/matriz_elmejor.txt");
    if (laberinto.empty()) return -1;

    for (int y = 0; y < (int)laberinto.size(); y++) {
        for (int x = 0; x < (int)laberinto[0].size(); x++) {
            if (laberinto[y][x] == 2) waypoints.push_back(glm::ivec2(x, y));
        }
    }
    if (waypoints.empty()) waypoints.push_back(glm::ivec2(laberinto[0].size() / 2, laberinto.size() / 2));

    Model sotanoModel("Resources/sotanoTexturas.obj");
    Model esferaModel("Resources/sphere/scene.gltf");

    glm::vec3 posActualIA = glm::vec3(
        OFFSET_X + (waypoints[0].x * TAMANO_BLOQUE) + CENTRO_BLOQUE,
        NIVEL_DEL_SUELO + ALTURA_ESFERA,
        -(OFFSET_Z + (waypoints[0].y * TAMANO_BLOQUE) + CENTRO_BLOQUE)
    );

    Pathfinding IA_Cerebro;
    IA_Cerebro.temperatura = 1.2f;
    float velocidadIA = 4.0f;

    std::vector<glm::ivec2> rutaActual;
    int indiceRuta = 0;

    auto AsignarNuevaMision = [&]() {
        glm::ivec2 posMatrizIA(
            (int)floor((posActualIA.x - OFFSET_X) / TAMANO_BLOQUE),
            (int)floor((-posActualIA.z - OFFSET_Z) / TAMANO_BLOQUE)
        );
        glm::ivec2 nuevoDestino;
        do { nuevoDestino = waypoints[rand() % waypoints.size()]; } while (nuevoDestino == posMatrizIA && waypoints.size() > 1);

        rutaActual = IA_Cerebro.EncontrarCamino(posMatrizIA, nuevoDestino, laberinto);
        indiceRuta = 1;
        };

    AsignarNuevaMision();

    // ==========================================
    // ¡AQUÍ ESTÁS NACIENDO TÚ!
    // ==========================================
    camera.Position = glm::vec3(SPAWN_X, SPAWN_Y, SPAWN_Z);

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

            std::cout << "\n--- ESTADO DEL JUGADOR (USA ESTO PARA TU SPAWN) ---" << std::endl;
            std::cout << "Mundo 3D -> X: " << camera.Position.x << " | Y (Altura): " << camera.Position.y << " | Z: " << camera.Position.z << std::endl;
            std::cout << "Matriz 2D-> Columna(X): " << cX << " | Fila(Z): " << cZ << std::endl;

            if (cZ >= 0 && cZ < laberinto.size() && cX >= 0 && cX < laberinto[0].size()) {
                int valorMatriz = laberinto[cZ][cX];
                std::cout << "Casilla actual: [" << valorMatriz << "] ";
                if (valorMatriz == 1) std::cout << "(ZONA DE COLISION)" << std::endl;
                else std::cout << "(Espacio Libre Caminable)" << std::endl;
            }
            else {
                std::cout << "FUERA DE LOS LIMITES DEL MAPA!" << std::endl;
            }
            std::cout << "--------------------------\n" << std::endl;
            debugTimer = 0.0f;
        }

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (indiceRuta < rutaActual.size()) {
            glm::vec3 nodoObjetivo3D(
                OFFSET_X + (rutaActual[indiceRuta].x * TAMANO_BLOQUE) + CENTRO_BLOQUE,
                NIVEL_DEL_SUELO + ALTURA_ESFERA,
                -(OFFSET_Z + (rutaActual[indiceRuta].y * TAMANO_BLOQUE) + CENTRO_BLOQUE)
            );

            glm::vec3 direccion = nodoObjetivo3D - posActualIA;
            float distanciaAlNodo = glm::length(direccion);
            float pasoDeMovimiento = velocidadIA * deltaTime;

            if (distanciaAlNodo > pasoDeMovimiento) {
                direccion = glm::normalize(direccion);
                posActualIA += direccion * pasoDeMovimiento;
            }
            else {
                posActualIA = nodoObjetivo3D;
                indiceRuta++;
            }
        }
        else {
            AsignarNuevaMision();
        }

        shader.use();
        glUniform3f(glGetUniformLocation(shader.ID, "lightPos"), 50.0f, 50.0f, 50.0f);
        glUniform3f(glGetUniformLocation(shader.ID, "viewPos"), camera.Position.x, camera.Position.y, camera.Position.z);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(camera.GetViewMatrix()));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f)));

        glUniform3f(glGetUniformLocation(shader.ID, "colorTint"), 1.0f, 1.0f, 1.0f);

        glm::mat4 sotanoMat = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(sotanoMat));
        sotanoModel.Draw(shader.ID);

        glm::mat4 monsterMat = glm::mat4(1.0f);
        monsterMat = glm::translate(monsterMat, posActualIA);
        monsterMat = glm::scale(monsterMat, glm::vec3(0.1f));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(monsterMat));
        esferaModel.Draw(shader.ID);

        // --- DEBUGGER VISUAL (SOLO COLISIONES) ---
        for (int f = 0; f < laberinto.size(); f++) {
            for (int c = 0; c < laberinto[0].size(); c++) {

                float posX = OFFSET_X + (c * TAMANO_BLOQUE) + CENTRO_BLOQUE;
                float blenderY = OFFSET_Z + (f * TAMANO_BLOQUE) + CENTRO_BLOQUE;
                float posZ = -blenderY;

                float distanciaAlJugador = sqrt(pow(camera.Position.x - posX, 2) + pow(camera.Position.z - posZ, 2));

                if (distanciaAlJugador < 12.0f) {

                    if (laberinto[f][c] == 1) {
                        // PARED (Bolas Verdes) - A la altura del pecho (1.3f) y gigantes (0.08f)
                        glm::mat4 debugMat = glm::mat4(1.0f);
                        debugMat = glm::translate(debugMat, glm::vec3(posX, NIVEL_DEL_SUELO + 1.3f, posZ));
                        debugMat = glm::scale(debugMat, glm::vec3(0.08f));

                        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(debugMat));
                        glUniform3f(glGetUniformLocation(shader.ID, "colorTint"), 0.0f, 1.0f, 0.0f);
                        esferaModel.Draw(shader.ID);
                    }
                    // TODO EL BLOQUE 'ELSE' CON LAS BOLAS MARRONES HA SIDO ELIMINADO AQUÍ
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