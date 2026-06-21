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

#include "Raycast.h"
#include "Shader.h"
#include "Model.h"
#include "Player.h"
#include "LightSystem.h"
#include "AudioManager.h"
#include "Door.h"
#include "Menu.h"
#include "ItemSystem.h"
#include "HUD.h"
#include "Switch.h"
#include "Intro.h"


const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int  currentWindowWidth = SCR_WIDTH;
int  currentWindowHeight = SCR_HEIGHT;

bool isFullscreen = false;
bool mPressedLastFrame = false;

int windowedPosX = 100;
int windowedPosY = 100;
int windowedWidth = SCR_WIDTH;
int windowedHeight = SCR_HEIGHT;

Player       player(glm::vec3(-56.0f, 2.0f, -86.807f));
AudioManager audio;

bool  ePressedLastFrame = false;

float nextTensionSoundTime = 0.0f;
float nextLaughSoundTime = 0.0f;
float nextDoorKnockTime = 0.0f;

bool pPressedLastFrame = false;

const float OFFSET_X = -76.40f;
const float OFFSET_Z = -24.00f;
const float TAMANO_BLOQUE = 0.8f;
const float CENTRO_BLOQUE = TAMANO_BLOQUE / 2.0f;
const float NIVEL_DEL_SUELO = 2.27f;

const float MONSTER_HEIGHT = 2.3f;
const float MONSTER_SPEED = 15.0f;

std::vector<std::vector<int>> laberinto;

// ==================================================
// Helpers
// ==================================================
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
        else if (isdigit(c)) { archivo.putback(c); archivo >> num; filaActual.push_back(num); }
        else if (c == ',') continue;
    }
    return matriz;
}

float RandomRange(float min, float max)
{
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

// ==================================================
// Callbacks
// ==================================================
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
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    player.camera.ProcessMouseMovement(xpos - lastX, lastY - ypos);
    lastX = xpos; lastY = ypos;
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
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        glViewport(0, 0, mode->width, mode->height);
    }
    else
    {
        currentWindowWidth = windowedWidth;
        currentWindowHeight = windowedHeight;
        glfwSetWindowMonitor(window, NULL, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0);
        glViewport(0, 0, windowedWidth, windowedHeight);
    }
    firstMouse = true;
}

void CheckFullscreenKey(GLFWwindow* window)
{
    bool mPressed = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
    if (mPressed && !mPressedLastFrame) ToggleFullscreen(window);
    mPressedLastFrame = mPressed;
}

float GetAspectRatio()
{
    if (currentWindowHeight <= 0) return (float)SCR_WIDTH / (float)SCR_HEIGHT;
    return (float)currentWindowWidth / (float)currentWindowHeight;
}

// ==================================================
// MAIN
// ==================================================
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hunted", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { glfwTerminate(); return -1; }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    srand((unsigned int)time(NULL));

    // ==================================================
    // AUDIO
    // ==================================================
    if (!audio.Init())
        std::cout << "ERROR: No se pudo iniciar el sistema de audio." << std::endl;
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

    // ==================================================
    // MATRIZ DE COLISIONES
    // ==================================================
    laberinto = cargarLaberinto("Resources/Models/Casa/matriz_tremenuwu.txt");
    if (laberinto.empty())
        std::cout << "ADVERTENCIA: Matriz vacia." << std::endl;
    else
        std::cout << "Matriz cargada (" << laberinto.size() << " filas)." << std::endl;

    // ==================================================
    // MONSTRUO — waypoints
    // ==================================================
    Pathfinding iaMonstruo;
    iaMonstruo.temperatura = 0.3f;

    std::vector<glm::ivec2> waypointsMonstruo;
    if (!laberinto.empty())
        for (int f = 0; f < (int)laberinto.size(); ++f)
            for (int c = 0; c < (int)laberinto[f].size(); ++c)
                if (laberinto[f][c] == 2)
                    waypointsMonstruo.push_back(glm::ivec2(c, f));

    std::vector<glm::ivec2> waypointsUnicos;
    if (!waypointsMonstruo.empty())
    {
        std::vector<bool> usado(waypointsMonstruo.size(), false);
        for (size_t i = 0; i < waypointsMonstruo.size(); ++i)
        {
            if (usado[i]) continue;
            waypointsUnicos.push_back(waypointsMonstruo[i]);
            for (size_t j = i + 1; j < waypointsMonstruo.size(); ++j)
                if (!usado[j] && glm::length(glm::vec2(waypointsMonstruo[j] - waypointsMonstruo[i])) < 2.0f)
                    usado[j] = true;
        }
    }

    glm::vec3              monsterPos(129.0f, MONSTER_HEIGHT, -98.0f);
    std::vector<glm::vec3> rutaSuaveMundo;
    size_t                 indiceRutaActual = 0;

    if (!laberinto.empty() && !waypointsUnicos.empty())
    {
        glm::ivec2 destino = waypointsUnicos[rand() % waypointsUnicos.size()];
        int sC = std::max(0, std::min((int)floor((monsterPos.x - OFFSET_X) / TAMANO_BLOQUE), (int)laberinto[0].size() - 1));
        int sF = std::max(0, std::min((int)floor((-monsterPos.z - OFFSET_Z) / TAMANO_BLOQUE), (int)laberinto.size() - 1));
        auto cruda = iaMonstruo.PlanificarRuta(glm::ivec2(sC, sF), destino, laberinto, iaMonstruo.temperatura);
        auto suave = iaMonstruo.SuavizarCamino(cruda, laberinto);
        for (auto& p : suave)
            rutaSuaveMundo.push_back(glm::vec3(
                OFFSET_X + p.x * TAMANO_BLOQUE + CENTRO_BLOQUE,
                MONSTER_HEIGHT,
                -(OFFSET_Z + p.y * TAMANO_BLOQUE + CENTRO_BLOQUE)));
    }

    // ==================================================
    // SHADERS
    // ==================================================
    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    Shader lampShader("shaders/lamp.vert", "shaders/lamp.frag");
    Shader depthShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");

    // ==================================================
    // MENU + carga de modelos
    // ==================================================
    Menu menu(SCR_WIDTH, SCR_HEIGHT);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (!audio.IsPlaying("intro")) audio.Play("intro");

    menu.state = MenuState::LOADING;
    Model* model = nullptr;
    Model* monsterModel = nullptr;
    float  fakeProgress = 0.0f;
    float  loadLastFrame = (float)glfwGetTime();
    bool   modelLoaded = false;

    while (!glfwWindowShouldClose(window) && !modelLoaded)
    {
        float now = (float)glfwGetTime();
        float dt = now - loadLastFrame;
        loadLastFrame = now;

        CheckFullscreenKey(window);

        if (fakeProgress < 0.9f) { fakeProgress += dt * 0.30f; if (fakeProgress > 0.9f) fakeProgress = 0.9f; }
        menu.SetLoadingProgress(fakeProgress);
        menu.blinkTimer += dt;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        menu.Render();
        glfwSwapBuffers(window);
        glfwPollEvents();

        if (fakeProgress >= 0.9f)
        {
            model = new Model("Resources/Models/Casa/sotanoCorregido.obj");
            monsterModel = new Model("Resources/Models/Casa/sphere/scene.gltf");
            modelLoaded = true;
            menu.SetLoadingProgress(1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            menu.Render();
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

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
            menu.renderSettings(window);
        else if (menu.state == MenuState::LOADING)
        {
            menu.loadingProgress += dt * 0.8f;
            menu.blinkTimer += dt;
            if (menu.loadingProgress >= 1.0f) menu.state = MenuState::PLAYING;
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
        delete model; delete monsterModel;
        audio.Shutdown(); glfwTerminate(); return 0;
    }
  // ==================================================
// INTRO CINEMATICA
// ==================================================
Intro intro(SCR_WIDTH, SCR_HEIGHT);
    intro.Init("Resources/Fonts/times.ttf");

    if (audio.IsPlaying("intro")) audio.Stop("intro");

    std::ifstream testFile("Resources/Audio/ambienteIntro.mp3");
    std::cout << "[DEBUG] El archivo existe en esa ruta? " << (testFile.good() ? "SI" : "NO") << std::endl;
    testFile.close();
    bool okMusica = audio.LoadSound("musica_intro", "Resources/Audio/ambienteIntro.mp3", true);
    std::cout << "[DEBUG] Carga de musica_intro: " << (okMusica ? "OK" : "FALLO") << std::endl;
    audio.SetVolume("musica_intro", 0.5f);
    audio.Play("musica_intro");

    float introLastFrame = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window) && !intro.finished)
    {
        float now = (float)glfwGetTime();
        float dt = now - introLastFrame;
        introLastFrame = now;

        CheckFullscreenKey(window);

        // Saltar intro con ESPACIO o ENTER
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
            intro.finished = true;

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        intro.Update(dt, &audio);
        intro.Render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    audio.Stop("musica_intro");

    if (audio.IsPlaying("intro")) audio.Stop("intro");
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    firstMouse = true;
    player.camera.MouseSensitivity = menu.mouseSensitivity;
    lastFrame = (float)glfwGetTime();



    // ==================================================
    // LUCES
    // ==================================================
    LightSystem lightSystem;

    // ==================================================
    // PUERTAS
    // ==================================================
    std::vector<std::unique_ptr<Door>> doors;

    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_1.obj",
        glm::vec3(29.264f, 0.0f, 3.9708f),
        glm::vec3(28.2658f, 1.6f, 2.04128f),
        0.0f, 90.0f, 120.0f, 4.0f, true));

    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_2.obj",
        glm::vec3(-18.673f, 0.0f, 3.9345f),
        glm::vec3(-18.673f, 1.6f, 3.9345f),
        0.0f, 90.0f, 120.0f, 6.0f, true));

    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_3.obj",
        glm::vec3(-46.39f, 0.0f, -89.064f),
        glm::vec3(-46.39f, 1.6f, -89.064f),
        0.0f, -90.0f, 120.0f, 6.0f, false));

    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_4.obj",
        glm::vec3(33.317f, 0.0f, -84.054f),
        glm::vec3(33.317f, 1.6f, -84.054f),
        0.0f, 90.0f, 120.0f, 6.0f, false));

    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_5.obj",
        glm::vec3(53.266f, 0.0f, -107.91f),
        glm::vec3(53.266f, 1.6f, -107.91f),
        0.0f, 90.0f, 120.0f, 6.0f, true));

// ==================================================
// SWITCHES DE LUZ
// ==================================================
    std::vector<std::unique_ptr<LightSwitch>> switches;

    // Switch 1: controla lamparas [26], [35] y [36]

    switches.push_back(std::make_unique<LightSwitch>(
        "Resources/Models/switch/switch_base.obj",
        "Resources/Models/switch/switch_rocker.obj",
        glm::vec3(-46.5f, 2.5f, -85.8093f),
        std::vector<int>{26, 35, 36},
        3.0f,
        0.15f,
        true,
        180.0f    // <-- rotacion para esta pared
    ));

    switches.push_back(std::make_unique<LightSwitch>(
        "Resources/Models/switch/switch_base.obj",
        "Resources/Models/switch/switch_rocker.obj",
        glm::vec3(-6.3f, 2.5f, -111.58f),
        std::vector<int>{26},
        3.0f,
        0.15f,
        true,
        -90.0f   // <-- rotacion para esta pared
    ));
    
    //switch cuarto sin puerta1
    switches.push_back(std::make_unique<LightSwitch>(
        "Resources/Models/switch/switch_base.obj",
        "Resources/Models/switch/switch_rocker.obj",
        glm::vec3(-56.35, 2.5f, -5.0),
        std::vector<int>{9, 10},
        3.0f,
        0.15f,
        true,
        0.0f   // <-- rotacion para esta pared
    ));
    //switch puerta llave1
    switches.push_back(std::make_unique<LightSwitch>(
        "Resources/Models/switch/switch_base.obj",
        "Resources/Models/switch/switch_rocker.obj",
        glm::vec3(26.4432, 2.5f, 4.1),
        std::vector<int>{2, 3},
        3.0f,
        0.15f,
        true,
        90.0f,
        180.0f// <-- rotacion para esta pared
    ));

    //switch puerta llave2
    switches.push_back(std::make_unique<LightSwitch>(
        "Resources/Models/switch/switch_base.obj",
        "Resources/Models/switch/switch_rocker.obj",
        glm::vec3(-29.0, 2.5f, 23.57),
        std::vector<int>{1},
        3.0f,
        0.15f,
        true,
        90.0f   // <-- rotacion para esta pared
    ));


    
   /*---------------------------------------------------
            POCICION DE LAS LLAVES
    -----------------------------------------------------*/
    ItemSystem itemSystem;
    // Fallback si no hay celdas 3 en la matriz
    std::vector<glm::vec3> keySpawnPoints;
    // 4 pupitres posibles para la llave 1 (se elige uo al azar cada partida)
    keySpawnPoints.push_back(glm::vec3(-2.91f, 0.35f, -95.59f));
    keySpawnPoints.push_back(glm::vec3(-2.91f, 0.35f, -98.66f));
    keySpawnPoints.push_back(glm::vec3(-2.91f, 0.35f, -101.66f));
    keySpawnPoints.push_back(glm::vec3(-2.91f, 0.35f, -104.49f));

    std::vector<glm::vec3> key2SpawnPoints;
    key2SpawnPoints.push_back(glm::vec3(22.5f, -0.4f, 5.3f));

    std::vector<glm::vec3> key3SpawnPoints;
    key3SpawnPoints.push_back(glm::vec3(94.0f, -0.4f, -140.0f));
    key3SpawnPoints.push_back(glm::vec3(95.6762f, -0.4, -138.887f));
    key3SpawnPoints.push_back(glm::vec3(141.882f, -0.4f, -93.5552f));
    key3SpawnPoints.push_back(glm::vec3(94.8381f, -0.4f, -94.0005f));

    itemSystem.SpawnKeyRandom(keySpawnPoints);
    itemSystem.SpawnKey2Random(key2SpawnPoints);
    itemSystem.SpawnKey3Random(key3SpawnPoints);


    // Confirmar posicion final de la llave
    for (auto& item : itemSystem.items)
        std::cout << "[DEBUG KEY] pos=("
        << item.position.x << ", "
        << item.position.y << ", "
        << item.position.z << ") visible="
        << item.visible << std::endl;

    // ==================================================
    // HUD
    // ==================================================
    HUD hud;
    if (!hud.Init("Resources/Fonts/arial.ttf", currentWindowWidth, currentWindowHeight))
        std::cout << "[ADVERTENCIA] HUD sin fuente. Pon un .ttf en Resources/Fonts/" << std::endl;

    float keyPickedTimer = 0.0f;
    float key2PickedTimer = 0.0f;
    float key3PickedTimer = 0.0f;

    // ==================================================
    // SHADOW MAPS
    // ==================================================
    unsigned int flashDepthMapFBO, lampDepthMapFBO;
    unsigned int flashDepthMap, lampDepthMap;
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    auto makeShadowMap = [&](unsigned int& fbo, unsigned int& tex)
        {
            glGenFramebuffers(1, &fbo);
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                SHADOW_WIDTH, SHADOW_HEIGHT, 0,
                GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        };
    makeShadowMap(flashDepthMapFBO, flashDepthMap);
    makeShadowMap(lampDepthMapFBO, lampDepthMap);

    // ==================================================
    // CUBO PARA LAMPARAS / ITEMS
    // ==================================================
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

    // ==================================================
    // GAME LOOP
    // ==================================================
    // Timer para limitar el debug del raycast (solo 1 vez por segundo)
    float debugRayTimer = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        CheckFullscreenKey(window);

        // --- Jugador ---
        glm::vec3 oldPos = player.camera.Position;
        player.ProcessInput(window, deltaTime);
        player.UpdatePhysics(deltaTime);
        player.UpdateFlashlight();

        // Colisiones jugador
        if (!laberinto.empty() && player.camera.Position.y < (NIVEL_DEL_SUELO + 3.0f))
        {
            float mg = 0.15f;
            float bz = -player.camera.Position.z;
            int fMin = (int)floor(((bz - mg) - OFFSET_Z) / TAMANO_BLOQUE);
            int fMax = (int)floor(((bz + mg) - OFFSET_Z) / TAMANO_BLOQUE);
            int cIzq = (int)floor(((player.camera.Position.x - mg) - OFFSET_X) / TAMANO_BLOQUE);
            int cDer = (int)floor(((player.camera.Position.x + mg) - OFFSET_X) / TAMANO_BLOQUE);
            if (fMin < 0 || fMax >= (int)laberinto.size() ||
                cIzq < 0 || cDer >= (int)laberinto[0].size() ||
                laberinto[fMin][cIzq] == 1 || laberinto[fMin][cDer] == 1 ||
                laberinto[fMax][cIzq] == 1 || laberinto[fMax][cDer] == 1)
            {
                player.camera.Position.x = oldPos.x;
                player.camera.Position.z = oldPos.z;
            }
        }

        // --- Monstruo ---
        glm::vec3 oldMonsterPos = monsterPos;
        if (!rutaSuaveMundo.empty() && indiceRutaActual < rutaSuaveMundo.size())
        {
            glm::vec3 dir = rutaSuaveMundo[indiceRutaActual] - monsterPos;
            float     dist = glm::length(dir);
            if (dist > 0.1f) monsterPos += glm::normalize(dir) * MONSTER_SPEED * deltaTime;
            else             indiceRutaActual++;
        }
        else if (!waypointsUnicos.empty() && !laberinto.empty())
        {
            glm::ivec2 destino = waypointsUnicos[rand() % waypointsUnicos.size()];
            int sC = std::max(0, std::min((int)floor((monsterPos.x - OFFSET_X) / TAMANO_BLOQUE), (int)laberinto[0].size() - 1));
            int sF = std::max(0, std::min((int)floor((-monsterPos.z - OFFSET_Z) / TAMANO_BLOQUE), (int)laberinto.size() - 1));
            auto cruda = iaMonstruo.PlanificarRuta(glm::ivec2(sC, sF), destino, laberinto, iaMonstruo.temperatura);
            auto suave = iaMonstruo.SuavizarCamino(cruda, laberinto);
            rutaSuaveMundo.clear();
            for (auto& p : suave)
                rutaSuaveMundo.push_back(glm::vec3(
                    OFFSET_X + p.x * TAMANO_BLOQUE + CENTRO_BLOQUE,
                    MONSTER_HEIGHT,
                    -(OFFSET_Z + p.y * TAMANO_BLOQUE + CENTRO_BLOQUE)));
            indiceRutaActual = 0;
        }

        // Colisiones monstruo
        if (!laberinto.empty())
        {
            float mg = 0.08f;
            float bz = -monsterPos.z;
            int fMin = (int)floor(((bz - mg) - OFFSET_Z) / TAMANO_BLOQUE);
            int fMax = (int)floor(((bz + mg) - OFFSET_Z) / TAMANO_BLOQUE);
            int cIzq = (int)floor(((monsterPos.x - mg) - OFFSET_X) / TAMANO_BLOQUE);
            int cDer = (int)floor(((monsterPos.x + mg) - OFFSET_X) / TAMANO_BLOQUE);
            bool col =
                fMin < 0 || fMax >= (int)laberinto.size() ||
                cIzq < 0 || cDer >= (int)laberinto[0].size() ||
                laberinto[fMin][cIzq] == 1 || laberinto[fMin][cDer] == 1 ||
                laberinto[fMax][cIzq] == 1 || laberinto[fMax][cDer] == 1;

            bool cerca = false;
            if (!rutaSuaveMundo.empty() && indiceRutaActual < rutaSuaveMundo.size() &&
                glm::length(monsterPos - rutaSuaveMundo[indiceRutaActual]) < 0.5f)
            {
                auto  pt = rutaSuaveMundo[indiceRutaActual];
                int   pc = (int)floor((pt.x - OFFSET_X) / TAMANO_BLOQUE);
                int   pf = (int)floor((-pt.z - OFFSET_Z) / TAMANO_BLOQUE);
                if (pf >= 0 && pf < (int)laberinto.size() &&
                    pc >= 0 && pc < (int)laberinto[0].size() &&
                    laberinto[pf][pc] == 0)
                    cerca = true;
            }
            if (col && !cerca) monsterPos = oldMonsterPos;
        }

        // Debug monstruo (cada 1 segundo)
        static float debugTimer = 0.0f;
        debugTimer += deltaTime;
        if (debugTimer >= 1.0f)
        {
            debugTimer = 0.0f;
            int cx = (int)floor((monsterPos.x - OFFSET_X) / TAMANO_BLOQUE);
            int cz = (int)floor((-monsterPos.z - OFFSET_Z) / TAMANO_BLOQUE);
            if (cz >= 0 && cz < (int)laberinto.size() &&
                cx >= 0 && cx < (int)laberinto[0].size())
                std::cout << "[MONSTRUO] (" << monsterPos.x << "," << monsterPos.z
                << ") [" << cz << "," << cx << "]=" << laberinto[cz][cx] << std::endl;
        }

        // Debug luces (tecla P)
        bool pPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (pPressed && !pPressedLastFrame)
        {
            std::cout << "\n--- Luces cercanas ---\nJugador: "
                << player.camera.Position.x << ","
                << player.camera.Position.y << ","
                << player.camera.Position.z << std::endl;
            for (int i = 0; i < NUM_LAMPS; i++)
            {
                float d = glm::length(player.camera.Position - lightSystem.lampPositions[i]);
                if (d < 35.0f) std::cout << "Luz[" << i << "] d=" << d << std::endl;
            }
            std::cout << "----------------------\n";
        }
        pPressedLastFrame = pPressed;

        // --- Audio de movimiento ---
        bool moving = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        bool running = moving && (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

        if (running)
        {
            if (audio.IsPlaying("caminata")) audio.Stop("caminata");
            if (!audio.IsPlaying("correr"))   audio.Play("correr");
        }
        else if (moving)
        {
            if (audio.IsPlaying("correr"))   audio.Stop("correr");
            if (!audio.IsPlaying("caminata")) audio.Play("caminata");
        }
        else
        {
            if (audio.IsPlaying("caminata")) audio.Stop("caminata");
            if (audio.IsPlaying("correr"))   audio.Stop("correr");
        }

        // ==================================================
        // INTERACCION: ITEMS + PUERTAS
        // ==================================================
        bool ePressed = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;

        bool hadKeyBefore = itemSystem.hasKey;
        bool hadKey2Before = itemSystem.hasKey2;
        bool hadKey3Before = itemSystem.hasKey3;

        itemSystem.Update(deltaTime, player.camera.Position, player.camera.Front,
            ePressed, ePressedLastFrame, &audio);

        if (!hadKeyBefore && itemSystem.hasKey)  keyPickedTimer = 3.0f;
        if (!hadKey2Before && itemSystem.hasKey2) key2PickedTimer = 3.0f;
        if (!hadKey3Before && itemSystem.hasKey3) key3PickedTimer = 3.0f;

        if (keyPickedTimer > 0.0f) keyPickedTimer -= deltaTime;
        if (key2PickedTimer > 0.0f) key2PickedTimer -= deltaTime;
        if (key3PickedTimer > 0.0f) key3PickedTimer -= deltaTime;

        for (size_t i = 0; i < doors.size(); ++i)   
        {
            bool keyForThisDoor;

            if (i == 0)
                keyForThisDoor = itemSystem.hasKey;   // puerta_1 -> llave 1
            else if (i == 1)
                keyForThisDoor = itemSystem.hasKey2;  // puerta_2 -> llave 2
            else if (i == 4)
                keyForThisDoor = itemSystem.hasKey3;  // puerta_5 -> llave 3
            else
                keyForThisDoor = false;

            doors[i]->Update(deltaTime, player.camera.Position, player.camera.Front,
                ePressed, ePressedLastFrame, &audio, keyForThisDoor);
        }


        for (auto& sw : switches)
            sw->Update(deltaTime, player.camera.Position, player.camera.Front,
                ePressed, ePressedLastFrame, &audio, lightSystem.lampEnabled);

        ePressedLastFrame = ePressed;

       

        

        // ==================================================
        // ESTADO DEL HUD
        // ==================================================
        HUDState hudState;
        hudState.hasKey = itemSystem.hasKey;
        hudState.hasKey2 = itemSystem.hasKey2;
        hudState.hasKey3 = itemSystem.hasKey3;

        hudState.showKeyPickedMsg = (keyPickedTimer > 0.0f);
        hudState.keyPickedMsgTimer = keyPickedTimer;
        hudState.showKey2PickedMsg = (key2PickedTimer > 0.0f);
        hudState.key2PickedMsgTimer = key2PickedTimer;
        hudState.showKey3PickedMsg = (key3PickedTimer > 0.0f);
        hudState.key3PickedMsgTimer = key3PickedTimer;

        hudState.stamina = player.stamina;
        hudState.staminaMax = STAMINA_MAX;
        hudState.isExhausted = player.isExhausted;


        for (auto& sw : switches)
        {
            if (sw->isBeingLookedAt && !hudState.lookingAtSwitch)
            {
                hudState.lookingAtSwitch = true;
                hudState.switchIsOn = sw->isOn;
            }
        }
        /*
        // ==================================================
        // DEBUG RAYCAST — imprime 1 vez por segundo
        // ==================================================
        debugRayTimer += deltaTime;
        if (debugRayTimer >= 1.0f)
        {
            debugRayTimer = 0.0f;
            for (auto& item : itemSystem.items)
            {
                if (!item.visible) continue;
                float dist = glm::length(player.camera.Position - item.position);
                bool hit = Raycast::HitPointInRange(
                    player.camera.Position,
                    glm::normalize(player.camera.Front),
                    item.position,
                    item.lookRadius,
                    item.pickupRadius);

                std::cout << "[RAY] dist=" << dist
                    << " | hit=" << hit
                    << " | pickupRadius=" << item.pickupRadius
                    << " | lookRadius=" << item.lookRadius
                    << " | itemPos=("
                    << item.position.x << ", "
                    << item.position.y << ", "
                    << item.position.z << ")"
                    << " | playerPos=("
                    << player.camera.Position.x << ", "
                    << player.camera.Position.y << ", "
                    << player.camera.Position.z << ")"
                    << std::endl;
            }
        }*/

        // Detectar si el jugador mira un item recogible
        {
            glm::vec3 rd = glm::normalize(player.camera.Front);
            for (auto& item : itemSystem.items)
            {
                if (!item.visible) continue;
                if (glm::length(player.camera.Position - item.position) < item.pickupRadius &&
                    Raycast::HitPointInRange(player.camera.Position, rd,
                        item.position, item.lookRadius, item.pickupRadius))
                {
                    hudState.lookingAtItem = true;
                    break;
                }
            }
        }

        // Las puertas ya calculan isBeingLookedAt en su Update
        for (size_t i = 0; i < doors.size(); ++i)
        {
            if (doors[i]->isBeingLookedAt && !hudState.lookingAtDoor)
            {
                hudState.lookingAtDoor = true;
                hudState.doorIsOpen = doors[i]->IsOpen();

                hudState.doorRequiresKey = false;
                hudState.doorRequiresKey2 = false;
                hudState.doorRequiresKey3 = false;

                if (i == 0)
                    hudState.doorRequiresKey = doors[i]->RequiresKey();
                else if (i == 1)
                    hudState.doorRequiresKey2 = doors[i]->RequiresKey();
                else if (i == 4)
                    hudState.doorRequiresKey3 = doors[i]->RequiresKey();
            }
        }

        // ==================================================
        // EVENTOS DE TENSION
        // ==================================================
        if (currentFrame >= nextTensionSoundTime)
        {
            audio.Play("sonido_tension");
            nextTensionSoundTime = currentFrame + RandomRange(30.0f, 70.0f);
        }
        if (currentFrame >= nextLaughSoundTime)
        {
            audio.Play("risa_tension");
            nextLaughSoundTime = currentFrame + RandomRange(90.0f, 180.0f);
        }
        if (currentFrame >= nextDoorKnockTime)
        {
            audio.Play("toque_puerta");
            nextDoorKnockTime = currentFrame + 40.0f;
        }

        glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

        // --- Luces ---
        lightSystem.Update(deltaTime, currentFrame, player.camera.Position);
        if (lightSystem.lightsFlickering) { if (!audio.IsPlaying("sonido_luces")) audio.Play("sonido_luces"); }
        else { if (audio.IsPlaying("sonido_luces")) audio.Stop("sonido_luces"); }

        // --- Matrices de sombra ---
        glm::mat4 flashLightSpaceMatrix =
            glm::perspective(glm::radians(55.0f), 1.0f, 0.05f, 50.0f) *
            glm::lookAt(player.flashlight.position,
                player.flashlight.position + player.flashlight.direction,
                glm::vec3(0, 1, 0));

        glm::vec3 slp = lightSystem.lampPositions[26];
        glm::mat4 lampLightSpaceMatrix =
            glm::perspective(glm::radians(110.0f), 1.0f, 0.1f, 38.0f) *
            glm::lookAt(slp, slp + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));

        // ==================================================
        // DEPTH PASS — linterna
        // ==================================================
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, flashDepthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthShader.use();
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(flashLightSpaceMatrix));
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
        model->Draw(depthShader.ID);
        for (auto& d : doors) d->Draw(depthShader.ID, modelMat);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ==================================================
        // DEPTH PASS — lampara 26
        // ==================================================
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, lampDepthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthShader.use();
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lampLightSpaceMatrix));
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
        model->Draw(depthShader.ID);
        for (auto& d : doors) d->Draw(depthShader.ID, modelMat);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ==================================================
        // RENDER NORMAL
        // ==================================================
        glViewport(0, 0, currentWindowWidth, currentWindowHeight);
        glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = player.camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(player.camera.Zoom), GetAspectRatio(), 0.1f, 1000.0f);

        shader.use();
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "flashLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(flashLightSpaceMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "lampLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lampLightSpaceMatrix));
        glUniform1i(glGetUniformLocation(shader.ID, "lampShadowEnabled"), 1);

        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, flashDepthMap);
        glUniform1i(glGetUniformLocation(shader.ID, "flashShadowMap"), 10);

        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, lampDepthMap);
        glUniform1i(glGetUniformLocation(shader.ID, "lampShadowMap"), 11);

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
            std::string b = "lights[" + std::to_string(i) + "]";
            float lin = 0.045f, quad = 0.014f;
            float pwr = lightSystem.intensities[i] * 0.75f;
            float range = 13.0f;

            if (i == 2 || i == 3 || i == 4)
            {
                lin = 0.09f; quad = 0.032f; pwr = lightSystem.intensities[i] * 0.65f; range = 9.0f;
            }
            if (i == 29 || i == 34)
            {
                lin = 0.018f; quad = 0.0035f; pwr = lightSystem.intensities[i] * 1.5f; range = 24.0f;
            }
            if (i == 26)
            {
                lin = 0.026f; quad = 0.006f;  pwr = lightSystem.intensities[i] * 1.25f; range = 28.0f;
            }

            glUniform3f(glGetUniformLocation(shader.ID, (b + ".position").c_str()),
                lightSystem.lampPositions[i].x, lightSystem.lampPositions[i].y, lightSystem.lampPositions[i].z);
            glUniform3f(glGetUniformLocation(shader.ID, (b + ".color").c_str()), 1.0f, 0.95f, 0.8f);
            glUniform1f(glGetUniformLocation(shader.ID, (b + ".intensity").c_str()), pwr);
            glUniform1f(glGetUniformLocation(shader.ID, (b + ".constant").c_str()), 1.0f);
            glUniform1f(glGetUniformLocation(shader.ID, (b + ".linear").c_str()), lin);
            glUniform1f(glGetUniformLocation(shader.ID, (b + ".quadratic").c_str()), quad);
            glUniform1f(glGetUniformLocation(shader.ID, (b + ".range").c_str()), range);
        }

        glUniform3f(glGetUniformLocation(shader.ID, "viewPos"),
            player.camera.Position.x, player.camera.Position.y, player.camera.Position.z);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));

        glUniform3f(glGetUniformLocation(shader.ID, "highlightColor"), 0.0f, 0.0f, 0.0f);
        model->Draw(shader.ID);

        for (auto& d : doors)
            d->Draw(shader.ID, modelMat);

        /*-----------------------------------------------
              DIBUJAS LOS SWITCHES DE LUZ
        -----------------------------------------------*/

        for (auto& sw : switches)
            sw->Draw(shader.ID, modelMat);


        // ==================================================
        // MONSTRUO
        // ==================================================
        lampShader.use();
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 mm = glm::scale(glm::translate(glm::mat4(1.0f), monsterPos), glm::vec3(0.12f));
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(mm));
        glUniform3f(glGetUniformLocation(lampShader.ID, "lampColor"), 1.0f, 0.2f, 0.2f);
        monsterModel->Draw(lampShader.ID);

        // ==================================================
        // LAMPARAS
        // ==================================================

        std::vector<int> lamparasActivas = {
         1, 2, 3, 6, 8, 9, 10,
         11, 13, 16, 18, 21, 23, 26,
         29, 34, 35, 36
        };

        glBindVertexArray(lampVAO);
        for (int i : lamparasActivas)
        {
            glm::mat4 lm = glm::translate(glm::mat4(1.0f), lightSystem.lampPositions[i]);
            glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lm));

            float br = lightSystem.intensities[i];

            if (!lightSystem.lampEnabled[i])
            {
                // Apagada por switch — cubo oscuro visible
                glUniform3f(glGetUniformLocation(lampShader.ID, "lampColor"), 0.05f, 0.05f, 0.05f);
            }
            else
            {
                // Encendida — cubo brillante
                glUniform3f(glGetUniformLocation(lampShader.ID, "lampColor"), br, 0.95f * br, 0.8f * br);
            }

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // ==================================================
        // ITEMS — usar shader normal para texturas e iluminacion
        // ==================================================
        itemSystem.Draw(shader.ID, view, projection, lampVAO, modelMat);

        // ==================================================
        // HUD
        // ==================================================
        hud.Resize(currentWindowWidth, currentWindowHeight);
        hud.Render(hudState);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ==================================================
    // LIMPIEZA
    // ==================================================
    delete model;
    delete monsterModel;
    glDeleteVertexArrays(1, &lampVAO);
    glDeleteBuffers(1, &lampVBO);
    glDeleteFramebuffers(1, &flashDepthMapFBO);
    glDeleteFramebuffers(1, &flashDepthMapFBO);
    glDeleteFramebuffers(1, &lampDepthMapFBO);
    glDeleteTextures(1, &flashDepthMap);
    glDeleteTextures(1, &lampDepthMap);
    audio.Shutdown();
    glfwTerminate();
    return 0;
}
