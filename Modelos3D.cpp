#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <memory>

#include <fstream>
#include <string>
#include "pathfinding.h"

#include "Shader.h"
#include "Model.h"
#include "Player.h"
#include "LightSystem.h"
#include "AudioManager.h"
#include "Door.h"
#include "Main.h"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int currentWindowWidth = SCR_WIDTH;
int currentWindowHeight = SCR_HEIGHT;

bool isFullscreen = false;
bool mPressedLastFrame = false;

int windowedPosX = 100;
int windowedPosY = 100;
int windowedWidth = SCR_WIDTH;
int windowedHeight = SCR_HEIGHT;

Player player(glm::vec3(129.0f, 5.0f, -98.0f));

AudioManager audio;

bool ePressedLastFrame = false;

float nextTensionSoundTime = 0.0f;
float nextLaughSoundTime = 0.0f;
float nextDoorKnockTime = 0.0f;

bool pPressedLastFrame = false;

// ==========================================
// CONFIGURACIÓN DE LA MATRIZ DE COLISIONES
// ==========================================
const float OFFSET_X = -76.40f;
const float OFFSET_Z = -24.00f;
const float TAMANO_BLOQUE = 0.8f;
const float CENTRO_BLOQUE = TAMANO_BLOQUE / 2.0f;
const float NIVEL_DEL_SUELO = 2.27f;

// ==================== CONFIGURACIÓN DEL MONSTRUO ====================
const float MONSTER_HEIGHT = 2.3f;
const float MONSTER_SPEED = 15.0f;
// ===================================================================

std::vector<std::vector<int>> laberinto;

// Función para cargar la matriz
std::vector<std::vector<int>> cargarLaberinto(const std::string& ruta)
{
    std::vector<std::vector<int>> matriz;
    std::ifstream archivo(ruta);
    if (!archivo.is_open())
    {
        std::cout << "--> ERROR CRITICO: No se encontro el archivo " << ruta << std::endl;
        return matriz;
    }
    char c; int num; std::vector<int> filaActual;
    while (archivo >> c)
    {
        if (c == '{') filaActual.clear();
        else if (c == '}') { if (!filaActual.empty()) matriz.push_back(filaActual); }
        else if (isdigit(c))
        {
            archivo.putback(c); archivo >> num; filaActual.push_back(num);
        }
        else if (c == ',') continue;
    }
    return matriz;
}

float RandomRange(float min, float max)
{
    float random = (float)rand() / (float)RAND_MAX;
    return min + random * (max - min);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    currentWindowWidth = width;
    currentWindowHeight = height;
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

    player.camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    player.camera.ProcessMouseScroll((float)yoffset);
}

void ToggleFullscreen(GLFWwindow* window)
{
    isFullscreen = !isFullscreen;

    if (isFullscreen)
    {
        glfwGetWindowPos(window, &windowedPosX, &windowedPosY);
        glfwGetWindowSize(window, &windowedWidth, &windowedHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        currentWindowWidth = mode->width;
        currentWindowHeight = mode->height;

        glfwSetWindowMonitor(
            window,
            monitor,
            0,
            0,
            mode->width,
            mode->height,
            mode->refreshRate
        );

        glViewport(0, 0, mode->width, mode->height);

        std::cout << "Pantalla completa activada: "
            << mode->width << "x" << mode->height << std::endl;
    }
    else
    {
        currentWindowWidth = windowedWidth;
        currentWindowHeight = windowedHeight;

        glfwSetWindowMonitor(
            window,
            NULL,
            windowedPosX,
            windowedPosY,
            windowedWidth,
            windowedHeight,
            0
        );

        glViewport(0, 0, windowedWidth, windowedHeight);

        std::cout << "Modo ventana activado: "
            << windowedWidth << "x" << windowedHeight << std::endl;
    }

    firstMouse = true;
}

void CheckFullscreenKey(GLFWwindow* window)
{
    bool mPressed = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;

    if (mPressed && !mPressedLastFrame)
    {
        ToggleFullscreen(window);
    }

    mPressedLastFrame = mPressed;
}

float GetAspectRatio()
{
    if (currentWindowHeight <= 0)
        return (float)SCR_WIDTH / (float)SCR_HEIGHT;

    return (float)currentWindowWidth / (float)currentWindowHeight;
}

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hunted", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    srand((unsigned int)time(NULL));

    // ==========================================================
    // AUDIO
    // ==========================================================
    if (!audio.Init())
    {
        std::cout << "ERROR: No se pudo iniciar el sistema de audio." << std::endl;
    }
    else
    {
        audio.LoadSound("intro", "Resources/Audio/intro.mp3", true);
        audio.LoadSound("caminata", "Resources/Audio/caminata.mp3", true);
        audio.LoadSound("correr", "Resources/Audio/correr.mp3", true);
        audio.LoadSound("abrir_puerta", "Resources/Audio/abrir_puerta.wav", false);
        audio.LoadSound("toque_puerta", "Resources/Audio/toque_puerta.wav", false);
        audio.LoadSound("sonido_luces", "Resources/Audio/sonido_luces.wav", true);
        audio.LoadSound("sonido_tension", "Resources/Audio/sonido_tension.mp3", false);
        audio.LoadSound("risa_tension", "Resources/Audio/risa_tension.mp3", false);
        audio.LoadSound("recoger", "Resources/Audio/recoger.mp3", false);

        audio.SetVolume("intro", 0.45f);
        audio.SetVolume("caminata", 0.35f);
        audio.SetVolume("correr", 0.45f);
        audio.SetVolume("abrir_puerta", 0.8f);
        audio.SetVolume("toque_puerta", 0.8f);
        audio.SetVolume("sonido_luces", 0.55f);
        audio.SetVolume("sonido_tension", 0.45f);
        audio.SetVolume("risa_tension", 0.35f);
        audio.SetVolume("recoger", 0.7f);

        nextTensionSoundTime = RandomRange(20.0f, 40.0f);
        nextLaughSoundTime = RandomRange(60.0f, 120.0f);
        nextDoorKnockTime = 40.0f;
    }

    // ==========================================================
    // CARGAR MATRIZ DE COLISIONES
    // ==========================================================
    std::cout << "[INFO] Cargando matriz de colisiones..." << std::endl;
    laberinto = cargarLaberinto("C:/Users/hashe/OneDrive/Escritorio/Graficauwu/Resources/Models/Casa/matriz_tremenuwu.txt");
    if (laberinto.empty())
    {
        std::cout << "ADVERTENCIA: La matriz de colisiones está vacía o no se pudo cargar. No habrá colisiones." << std::endl;
    }
    else
    {
        std::cout << "Matriz de colisiones cargada correctamente (" << laberinto.size() << " filas)." << std::endl;
    }

    // ==================== INICIALIZACIÓN DEL MONSTRUO ====================
    Pathfinding iaMonstruo;
    iaMonstruo.temperatura = 0.3f;

    std::vector<glm::ivec2> waypointsMonstruo;
    if (!laberinto.empty()) {
        for (int f = 0; f < (int)laberinto.size(); ++f)
            for (int c = 0; c < (int)laberinto[f].size(); ++c)
                if (laberinto[f][c] == 2)
                    waypointsMonstruo.push_back(glm::ivec2(c, f));
        std::cout << "[IA] Waypoints encontrados: " << waypointsMonstruo.size() << std::endl;
    }

    // --- AGRUPAR WAYPOINTS PARA DEJAR SOLO UNO POR GRUPO ---
    std::vector<glm::ivec2> waypointsUnicos;
    if (!waypointsMonstruo.empty()) {
        const float DISTANCIA_AGRUPACION = 2.0f; // celdas de distancia máxima para considerar mismo grupo
        std::vector<bool> usado(waypointsMonstruo.size(), false);
        for (size_t i = 0; i < waypointsMonstruo.size(); ++i) {
            if (usado[i]) continue;
            glm::ivec2 wp = waypointsMonstruo[i];
            waypointsUnicos.push_back(wp);
            // Marcar todos los que estén cerca
            for (size_t j = i + 1; j < waypointsMonstruo.size(); ++j) {
                if (usado[j]) continue;
                float dist = glm::length(glm::vec2(waypointsMonstruo[j] - wp));
                if (dist < DISTANCIA_AGRUPACION) {
                    usado[j] = true;
                }
            }
        }
        std::cout << "[IA] Waypoints únicos: " << waypointsUnicos.size() << std::endl;
    }
    // Ahora usaremos waypointsUnicos en lugar de waypointsMonstruo para los destinos
    // ================================================================

    // Posición inicial (misma que el jugador)
    glm::vec3 monsterPos(129.0f, MONSTER_HEIGHT, -98.0f);
    int currentWaypointIndex = 0;
    std::vector<glm::vec3> rutaSuaveMundo;
    size_t indiceRutaActual = 0;

    // Verificar que la posición inicial sea válida
    if (!laberinto.empty()) {
        int initC = (int)floor((monsterPos.x - OFFSET_X) / TAMANO_BLOQUE);
        int initF = (int)floor((-monsterPos.z - OFFSET_Z) / TAMANO_BLOQUE);
        if (initF >= 0 && initF < (int)laberinto.size() && initC >= 0 && initC < (int)laberinto[0].size()) {
            if (laberinto[initF][initC] == 1) {
                std::cout << "[ERROR] Posicion inicial del monstruo dentro de una pared!" << std::endl;
                if (!waypointsUnicos.empty()) {
                    glm::ivec2 wp = waypointsUnicos[rand() % waypointsUnicos.size()];
                    monsterPos.x = OFFSET_X + wp.x * TAMANO_BLOQUE + CENTRO_BLOQUE;
                    monsterPos.z = -(OFFSET_Z + wp.y * TAMANO_BLOQUE + CENTRO_BLOQUE);
                    std::cout << "[INFO] Monstruo reposicionado en waypoint." << std::endl;
                }
            }
        }
    }

    if (!waypointsUnicos.empty() && !laberinto.empty()) {
        glm::ivec2 destino = waypointsUnicos[rand() % waypointsUnicos.size()];

        int startC = (int)floor((monsterPos.x - OFFSET_X) / TAMANO_BLOQUE);
        int startF = (int)floor((-monsterPos.z - OFFSET_Z) / TAMANO_BLOQUE);
        startC = std::max(0, std::min(startC, (int)laberinto[0].size() - 1));
        startF = std::max(0, std::min(startF, (int)laberinto.size() - 1));

        std::vector<glm::ivec2> cruda = iaMonstruo.PlanificarRuta(glm::ivec2(startC, startF), destino, laberinto, iaMonstruo.temperatura);
        std::vector<glm::ivec2> suave = iaMonstruo.SuavizarCamino(cruda, laberinto);

        for (auto& p : suave) {
            float px = OFFSET_X + (p.x * TAMANO_BLOQUE) + CENTRO_BLOQUE;
            float pz = -(OFFSET_Z + (p.y * TAMANO_BLOQUE) + CENTRO_BLOQUE);
            rutaSuaveMundo.push_back(glm::vec3(px, MONSTER_HEIGHT, pz));
        }
        indiceRutaActual = 0;
    }
    // =====================================================================

    // ==========================================================
    // SHADERS
    // ==========================================================
    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    Shader lampShader("shaders/lamp.vert", "shaders/lamp.frag");
    Shader depthShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");

    // ==========================================================
    // MENU
    // ==========================================================
    Menu menu(SCR_WIDTH, SCR_HEIGHT);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!audio.IsPlaying("intro"))
    {
        audio.Play("intro");
    }

    // ==========================================================
    // PANTALLA DE CARGA
    // ==========================================================
    menu.state = MenuState::LOADING;

    Model* model = nullptr;
    Model* monsterModel = nullptr;

    float fakeProgress = 0.0f;
    float loadLastFrame = (float)glfwGetTime();
    bool modelLoaded = false;

    while (!glfwWindowShouldClose(window) && !modelLoaded)
    {
        float now = (float)glfwGetTime();
        float dt = now - loadLastFrame;
        loadLastFrame = now;

        CheckFullscreenKey(window);

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
            model = new Model("Resources/Models/Casa/sotano2.obj");
            monsterModel = new Model("Resources/Models/Casa/sphere/scene.gltf");

            modelLoaded = true;

            menu.SetLoadingProgress(1.0f);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            menu.Render();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    // ==========================================================
    // MENU PRINCIPAL
    // ==========================================================
    menu.state = MenuState::MAIN;
    menu.loadingProgress = 0.0f;

    float menuLastFrame = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window) && menu.state != MenuState::PLAYING)
    {
        float now = (float)glfwGetTime();
        float dt = now - menuLastFrame;
        menuLastFrame = now;

        CheckFullscreenKey(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (menu.state == MenuState::SETTINGS)
        {
            menu.renderSettings(window);
        }
        else if (menu.state == MenuState::LOADING)
        {
            menu.loadingProgress += dt * 0.8f;
            menu.blinkTimer += dt;

            if (menu.loadingProgress >= 1.0f)
                menu.state = MenuState::PLAYING;

            menu.Render();
        }
        else
        {
            menu.Render();
        }

        menu.Update(window, dt);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (glfwWindowShouldClose(window))
    {
        delete model;
        delete monsterModel;
        audio.Shutdown();
        glfwTerminate();
        return 0;
    }

    // ==========================================================
    // ENTRAR AL JUEGO
    // ==========================================================
    if (audio.IsPlaying("intro"))
    {
        audio.Stop("intro");
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    firstMouse = true;

    player.camera.MouseSensitivity = menu.mouseSensitivity;

    lastFrame = (float)glfwGetTime();

    // ==========================================================
    // LUCES
    // ==========================================================
    LightSystem lightSystem;

    // ==========================================================
    // PUERTAS
    // ==========================================================
    std::vector<std::unique_ptr<Door>> doors;

    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_1.obj",
        glm::vec3(29.264f, 0.0f, 3.9708f),
        glm::vec3(28.2658f, 1.6f, 2.04128f),
        0.0f,
        90.0f,
        120.0f,
        4.0f
    ));

    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_2.obj",
        glm::vec3(-18.673f, 0.0f, 3.9345f),
        glm::vec3(-18.673f, 1.6f, 3.9345f),
        0.0f,
        90.0f,
        120.0f,
        6.0f
    ));

    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_3.obj",
        glm::vec3(-46.39f, 0.0f, -89.064f),
        glm::vec3(-46.39f, 1.6f, -89.064f),
        0.0f,
        -90.0f,
        120.0f,
        6.0f
    ));

    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_4.obj",
        glm::vec3(33.317f, 0.0f, -84.054f),
        glm::vec3(33.317f, 1.6f, -84.054f),
        0.0f,
        90.0f,
        120.0f,
        6.0f
    ));

    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_5.obj",
        glm::vec3(53.266f, 0.0f, -107.91f),
        glm::vec3(53.266f, 1.6f, -107.91f),
        0.0f,
        90.0f,
        120.0f,
        6.0f
    ));

    // ==========================================================
    // SHADOW MAPS
    // ==========================================================
    unsigned int flashDepthMapFBO;
    unsigned int lampDepthMapFBO;
    unsigned int flashDepthMap;
    unsigned int lampDepthMap;

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Shadow map linterna
    glGenFramebuffers(1, &flashDepthMapFBO);

    glGenTextures(1, &flashDepthMap);
    glBindTexture(GL_TEXTURE_2D, flashDepthMap);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT,
        SHADOW_WIDTH,
        SHADOW_HEIGHT,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        NULL
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, flashDepthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, flashDepthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Shadow map lámpara 26
    glGenFramebuffers(1, &lampDepthMapFBO);

    glGenTextures(1, &lampDepthMap);
    glBindTexture(GL_TEXTURE_2D, lampDepthMap);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT,
        SHADOW_WIDTH,
        SHADOW_HEIGHT,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        NULL
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, lampDepthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, lampDepthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ==========================================================
    // CUBO VISUAL PARA LÁMPARAS
    // ==========================================================
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

    unsigned int lampVAO;
    unsigned int lampVBO;

    glGenVertexArrays(1, &lampVAO);
    glGenBuffers(1, &lampVBO);

    glBindVertexArray(lampVAO);

    glBindBuffer(GL_ARRAY_BUFFER, lampVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ==========================================================
    // LOOP PRINCIPAL
    // ==========================================================
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();

        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        CheckFullscreenKey(window);

        glm::vec3 oldPos = player.camera.Position;

        player.ProcessInput(window, deltaTime);
        player.UpdatePhysics(deltaTime);
        player.UpdateFlashlight();

        // ==========================================================
        // LÓGICA DE COLISIONES (paredes) – jugador
        // ==========================================================
        if (!laberinto.empty())
        {
            if (player.camera.Position.y < (NIVEL_DEL_SUELO + 3.0f))
            {
                float margen = 0.15f;
                float blenderZ_cam = -player.camera.Position.z;

                int fMin = (int)floor(((blenderZ_cam - margen) - OFFSET_Z) / TAMANO_BLOQUE);
                int fMax = (int)floor(((blenderZ_cam + margen) - OFFSET_Z) / TAMANO_BLOQUE);
                int cIzq = (int)floor(((player.camera.Position.x - margen) - OFFSET_X) / TAMANO_BLOQUE);
                int cDer = (int)floor(((player.camera.Position.x + margen) - OFFSET_X) / TAMANO_BLOQUE);

                if (fMin < 0 || fMax >= (int)laberinto.size() ||
                    cIzq < 0 || cDer >= (int)laberinto[0].size() ||
                    laberinto[fMin][cIzq] == 1 || laberinto[fMin][cDer] == 1 ||
                    laberinto[fMax][cIzq] == 1 || laberinto[fMax][cDer] == 1)
                {
                    player.camera.Position.x = oldPos.x;
                    player.camera.Position.z = oldPos.z;
                }
            }
        }

        // ==================== ACTUALIZACIÓN DEL MONSTRUO ====================
        // Guardar posición anterior para revertir en caso de colisión
        glm::vec3 oldMonsterPos = monsterPos;

        if (!rutaSuaveMundo.empty() && indiceRutaActual < rutaSuaveMundo.size()) {
            glm::vec3 target = rutaSuaveMundo[indiceRutaActual];
            glm::vec3 dir = target - monsterPos;
            float dist = glm::length(dir);

            if (dist > 0.1f) {
                dir = glm::normalize(dir);
                monsterPos += dir * MONSTER_SPEED * deltaTime;
            }
            else {
                indiceRutaActual++;
            }
        }
        else if (!waypointsUnicos.empty() && !laberinto.empty()) {
            int nuevoWP = rand() % waypointsUnicos.size();
            glm::ivec2 destino = waypointsUnicos[nuevoWP];

            int startC = (int)floor((monsterPos.x - OFFSET_X) / TAMANO_BLOQUE);
            int startF = (int)floor((-monsterPos.z - OFFSET_Z) / TAMANO_BLOQUE);
            startC = std::max(0, std::min(startC, (int)laberinto[0].size() - 1));
            startF = std::max(0, std::min(startF, (int)laberinto.size() - 1));

            std::vector<glm::ivec2> cruda = iaMonstruo.PlanificarRuta(glm::ivec2(startC, startF), destino, laberinto, iaMonstruo.temperatura);
            std::vector<glm::ivec2> suave = iaMonstruo.SuavizarCamino(cruda, laberinto);

            rutaSuaveMundo.clear();
            for (auto& p : suave) {
                float px = OFFSET_X + (p.x * TAMANO_BLOQUE) + CENTRO_BLOQUE;
                float pz = -(OFFSET_Z + (p.y * TAMANO_BLOQUE) + CENTRO_BLOQUE);
                rutaSuaveMundo.push_back(glm::vec3(px, MONSTER_HEIGHT, pz));
            }
            indiceRutaActual = 0;
        }

        // --- Hitbox del monstruo (pequeño y "permisivo" cerca del objetivo) ---
        if (!laberinto.empty()) {
            float margenMonster = 0.08f;   // margen mínimo
            float blenderZ_monster = -monsterPos.z;

            int fMin = (int)floor(((blenderZ_monster - margenMonster) - OFFSET_Z) / TAMANO_BLOQUE);
            int fMax = (int)floor(((blenderZ_monster + margenMonster) - OFFSET_Z) / TAMANO_BLOQUE);
            int cIzq = (int)floor(((monsterPos.x - margenMonster) - OFFSET_X) / TAMANO_BLOQUE);
            int cDer = (int)floor(((monsterPos.x + margenMonster) - OFFSET_X) / TAMANO_BLOQUE);

            bool colisionPared = false;
            if (fMin < 0 || fMax >= (int)laberinto.size() ||
                cIzq < 0 || cDer >= (int)laberinto[0].size()) {
                colisionPared = true;   // fuera del mapa
            }
            else if (laberinto[fMin][cIzq] == 1 || laberinto[fMin][cDer] == 1 ||
                laberinto[fMax][cIzq] == 1 || laberinto[fMax][cDer] == 1) {
                colisionPared = true;
            }

            // Si estamos cerca del punto de ruta actual y ese punto está en celda 0, permitimos ignorar la colisión
            bool cercaDePuntoRuta = false;
            if (!rutaSuaveMundo.empty() && indiceRutaActual < rutaSuaveMundo.size()) {
                float distAlPunto = glm::length(monsterPos - rutaSuaveMundo[indiceRutaActual]);
                if (distAlPunto < 0.5f) {
                    // Verificar que la celda del punto sea transitable
                    glm::vec3 punto = rutaSuaveMundo[indiceRutaActual];
                    int pC = (int)floor((punto.x - OFFSET_X) / TAMANO_BLOQUE);
                    int pF = (int)floor((-punto.z - OFFSET_Z) / TAMANO_BLOQUE);
                    if (pF >= 0 && pF < (int)laberinto.size() && pC >= 0 && pC < (int)laberinto[0].size() &&
                        laberinto[pF][pC] == 0) {
                        cercaDePuntoRuta = true;
                    }
                }
            }

            if (colisionPared && !cercaDePuntoRuta) {
                monsterPos = oldMonsterPos;
                std::cout << "[MONSTRUO] Hitbox detecto pared. Revertido." << std::endl;
            }
        }

        // Debug: posición y celda cada segundo
        static float debugTimer = 0.0f;
        debugTimer += deltaTime;
        if (debugTimer >= 1.0f) {
            debugTimer = 0.0f;
            int cellX = (int)floor((monsterPos.x - OFFSET_X) / TAMANO_BLOQUE);
            int cellZ = (int)floor((-monsterPos.z - OFFSET_Z) / TAMANO_BLOQUE);
            if (cellZ >= 0 && cellZ < (int)laberinto.size() && cellX >= 0 && cellX < (int)laberinto[0].size()) {
                std::cout << "[MONSTRUO] Pos: (" << monsterPos.x << ", " << monsterPos.z << ") celda [" << cellZ << "," << cellX << "] = " << laberinto[cellZ][cellX] << std::endl;
            }
            else {
                std::cout << "[MONSTRUO] FUERA DEL MAPA!" << std::endl;
            }
        }
        // =====================================================================

        // ==========================================================
        // DEBUG CON P
        // ==========================================================
        bool pPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;

        if (pPressed && !pPressedLastFrame)
        {
            std::cout << "\n--- Luces cercanas al jugador ---" << std::endl;

            std::cout << "Posicion jugador: "
                << player.camera.Position.x << ", "
                << player.camera.Position.y << ", "
                << player.camera.Position.z << std::endl;

            for (int i = 0; i < NUM_LAMPS; i++)
            {
                float dist = glm::length(player.camera.Position - lightSystem.lampPositions[i]);

                if (dist < 35.0f)
                {
                    std::cout << "Luz [" << i << "] distancia: " << dist
                        << " posicion: "
                        << lightSystem.lampPositions[i].x << ", "
                        << lightSystem.lampPositions[i].y << ", "
                        << lightSystem.lampPositions[i].z
                        << std::endl;
                }
            }

            std::cout << "----------------------------------\n" << std::endl;
        }

        pPressedLastFrame = pPressed;

        // ==========================================================
        // AUDIO DE MOVIMIENTO
        // ==========================================================
        bool moving =
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

        bool running =
            moving &&
            (
                glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS
                );

        if (running)
        {
            if (audio.IsPlaying("caminata"))
                audio.Stop("caminata");

            if (!audio.IsPlaying("correr"))
                audio.Play("correr");
        }
        else if (moving)
        {
            if (audio.IsPlaying("correr"))
                audio.Stop("correr");

            if (!audio.IsPlaying("caminata"))
                audio.Play("caminata");
        }
        else
        {
            if (audio.IsPlaying("caminata"))
                audio.Stop("caminata");

            if (audio.IsPlaying("correr"))
                audio.Stop("correr");
        }

        // ==========================================================
        // INTERACCIÓN CON PUERTAS
        // ==========================================================
        bool ePressed = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;

        for (auto& door : doors)
        {
            door->Update(
                deltaTime,
                player.camera.Position,
                ePressed,
                ePressedLastFrame,
                &audio
            );
        }

        ePressedLastFrame = ePressed;

        // ==========================================================
        // EVENTOS DE TENSIÓN / AMBIENTE
        // ==========================================================
        if (currentFrame >= nextTensionSoundTime)
        {
            audio.Play("sonido_tension");
            nextTensionSoundTime = currentFrame + RandomRange(30.0f, 70.0f);
            std::cout << "Evento de tension" << std::endl;
        }

        if (currentFrame >= nextLaughSoundTime)
        {
            audio.Play("risa_tension");
            nextLaughSoundTime = currentFrame + RandomRange(90.0f, 180.0f);
            std::cout << "Risa de tension" << std::endl;
        }

        if (currentFrame >= nextDoorKnockTime)
        {
            audio.Play("toque_puerta");
            nextDoorKnockTime = currentFrame + 40.0f;
            std::cout << "Evento: toque de puerta" << std::endl;
        }

        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.0f, 0.0f));

        // ==========================================================
        // ACTUALIZAR LUCES
        // ==========================================================
        lightSystem.Update(deltaTime, currentFrame, player.camera.Position);

        if (lightSystem.lightsFlickering)
        {
            if (!audio.IsPlaying("sonido_luces"))
                audio.Play("sonido_luces");
        }
        else
        {
            if (audio.IsPlaying("sonido_luces"))
                audio.Stop("sonido_luces");
        }

        // ==========================================================
        // MATRICES DE SOMBRA
        // ==========================================================
        glm::mat4 flashProjection = glm::perspective(
            glm::radians(55.0f),
            1.0f,
            0.05f,
            50.0f
        );

        glm::mat4 flashView = glm::lookAt(
            player.flashlight.position,
            player.flashlight.position + player.flashlight.direction,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        glm::mat4 flashLightSpaceMatrix = flashProjection * flashView;

        glm::vec3 shadowLampPos = lightSystem.lampPositions[26];
        glm::vec3 shadowLampDir = glm::vec3(0.0f, -1.0f, 0.0f);

        glm::mat4 lampProjection = glm::perspective(
            glm::radians(110.0f),
            1.0f,
            0.1f,
            38.0f
        );

        glm::mat4 lampView = glm::lookAt(
            shadowLampPos,
            shadowLampPos + shadowLampDir,
            glm::vec3(0.0f, 0.0f, -1.0f)
        );

        glm::mat4 lampLightSpaceMatrix = lampProjection * lampView;

        // ==========================================================
        // DEPTH PASS - LINTERNA
        // ==========================================================
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, flashDepthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        depthShader.use();

        glUniformMatrix4fv(
            glGetUniformLocation(depthShader.ID, "lightSpaceMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(flashLightSpaceMatrix)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(depthShader.ID, "model"),
            1,
            GL_FALSE,
            glm::value_ptr(modelMat)
        );

        model->Draw(depthShader.ID);

        for (auto& door : doors)
        {
            door->Draw(depthShader.ID, modelMat);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ==========================================================
        // DEPTH PASS - LÁMPARA 26
        // ==========================================================
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, lampDepthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        depthShader.use();

        glUniformMatrix4fv(
            glGetUniformLocation(depthShader.ID, "lightSpaceMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(lampLightSpaceMatrix)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(depthShader.ID, "model"),
            1,
            GL_FALSE,
            glm::value_ptr(modelMat)
        );

        model->Draw(depthShader.ID);

        for (auto& door : doors)
        {
            door->Draw(depthShader.ID, modelMat);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, currentWindowWidth, currentWindowHeight);

        // ==========================================================
        // RENDER NORMAL
        // ==========================================================
        glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = player.camera.GetViewMatrix();

        float aspectRatio = GetAspectRatio();

        glm::mat4 projection = glm::perspective(
            glm::radians(player.camera.Zoom),
            aspectRatio,
            0.1f,
            1000.0f
        );

        shader.use();

        glUniformMatrix4fv(
            glGetUniformLocation(shader.ID, "flashLightSpaceMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(flashLightSpaceMatrix)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(shader.ID, "lampLightSpaceMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(lampLightSpaceMatrix)
        );

        glUniform1i(glGetUniformLocation(shader.ID, "lampShadowEnabled"), 1);

        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, flashDepthMap);
        glUniform1i(glGetUniformLocation(shader.ID, "flashShadowMap"), 10);

        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, lampDepthMap);
        glUniform1i(glGetUniformLocation(shader.ID, "lampShadowMap"), 11);

        glUniform1i(
            glGetUniformLocation(shader.ID, "flashlightOn"),
            player.flashlight.on
        );

        glUniform3f(
            glGetUniformLocation(shader.ID, "flashlightPos"),
            player.flashlight.position.x,
            player.flashlight.position.y,
            player.flashlight.position.z
        );

        glUniform3f(
            glGetUniformLocation(shader.ID, "flashlightDir"),
            player.flashlight.direction.x,
            player.flashlight.direction.y,
            player.flashlight.direction.z
        );

        for (int i = 0; i < NUM_LAMPS; i++)
        {
            std::string base = "lights[" + std::to_string(i) + "]";

            float linear = 0.045f;
            float quadratic = 0.014f;
            float lampPower = lightSystem.intensities[i] * 0.75f;
            float range = 13.0f;

            if (i == 2 || i == 3 || i == 4)
            {
                linear = 0.09f;
                quadratic = 0.032f;
                lampPower = lightSystem.intensities[i] * 0.65f;
                range = 9.0f;
            }

            if (i == 29 || i == 34)
            {
                linear = 0.018f;
                quadratic = 0.0035f;
                lampPower = lightSystem.intensities[i] * 1.5f;
                range = 24.0f;
            }

            if (i == 26)
            {
                linear = 0.026f;
                quadratic = 0.006f;
                lampPower = lightSystem.intensities[i] * 1.25f;
                range = 28.0f;
            }

            glUniform3f(
                glGetUniformLocation(shader.ID, (base + ".position").c_str()),
                lightSystem.lampPositions[i].x,
                lightSystem.lampPositions[i].y,
                lightSystem.lampPositions[i].z
            );

            glUniform3f(
                glGetUniformLocation(shader.ID, (base + ".color").c_str()),
                1.0f,
                0.95f,
                0.8f
            );

            glUniform1f(
                glGetUniformLocation(shader.ID, (base + ".intensity").c_str()),
                lampPower
            );

            glUniform1f(
                glGetUniformLocation(shader.ID, (base + ".constant").c_str()),
                1.0f
            );

            glUniform1f(
                glGetUniformLocation(shader.ID, (base + ".linear").c_str()),
                linear
            );

            glUniform1f(
                glGetUniformLocation(shader.ID, (base + ".quadratic").c_str()),
                quadratic
            );

            glUniform1f(
                glGetUniformLocation(shader.ID, (base + ".range").c_str()),
                range
            );
        }

        glUniform3f(
            glGetUniformLocation(shader.ID, "viewPos"),
            player.camera.Position.x,
            player.camera.Position.y,
            player.camera.Position.z
        );

        // Configurar view y projection antes de dibujar geometría
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

        // Dibujar modelo del sótano
        glUniformMatrix4fv(
            glGetUniformLocation(shader.ID, "model"),
            1,
            GL_FALSE,
            glm::value_ptr(modelMat)
        );
        model->Draw(shader.ID);

        // Dibujar puertas
        for (auto& door : doors)
        {
            door->Draw(shader.ID, modelMat);
        }

        // ==================== DIBUJAR MONSTRUO (esfera brillante, sin sombras) ====================
        lampShader.use();
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 monsterMat = glm::mat4(1.0f);
        monsterMat = glm::translate(monsterMat, monsterPos);
        monsterMat = glm::scale(monsterMat, glm::vec3(0.12f));

        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(monsterMat));
        // Color rojo intenso
        glUniform3f(glGetUniformLocation(lampShader.ID, "lampColor"), 1.0f, 0.2f, 0.2f);
        monsterModel->Draw(lampShader.ID);

        // ==========================================================
        // DIBUJAR CUBOS DE LÁMPARAS
        // ==========================================================
        lampShader.use();   // Ya está activo, solo aseguramos

        glUniformMatrix4fv(
            glGetUniformLocation(lampShader.ID, "view"),
            1,
            GL_FALSE,
            glm::value_ptr(view)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(lampShader.ID, "projection"),
            1,
            GL_FALSE,
            glm::value_ptr(projection)
        );

        glBindVertexArray(lampVAO);

        for (int i = 0; i < NUM_LAMPS; i++)
        {
            if (!lightSystem.lampEnabled[i])
                continue;

            glm::mat4 lampMat = glm::mat4(1.0f);
            lampMat = glm::translate(lampMat, lightSystem.lampPositions[i]);

            glUniformMatrix4fv(
                glGetUniformLocation(lampShader.ID, "model"),
                1,
                GL_FALSE,
                glm::value_ptr(lampMat)
            );

            float bright = lightSystem.intensities[i];

            glUniform3f(
                glGetUniformLocation(lampShader.ID, "lampColor"),
                1.0f * bright,
                0.95f * bright,
                0.8f * bright
            );

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete model;
    delete monsterModel;

    glDeleteVertexArrays(1, &lampVAO);
    glDeleteBuffers(1, &lampVBO);

    glDeleteFramebuffers(1, &flashDepthMapFBO);
    glDeleteFramebuffers(1, &lampDepthMapFBO);

    glDeleteTextures(1, &flashDepthMap);
    glDeleteTextures(1, &lampDepthMap);

    audio.Shutdown();

    glfwTerminate();

    return 0;
}