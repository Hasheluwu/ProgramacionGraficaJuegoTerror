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
#include <algorithm>
#include <array>
#include <cfloat>

#include <fstream>
#include <string>
#include <cctype>

#include "pathfinding.h"
#include "Raycast.h"
#include "Shader.h"
#include "Model.h"
#include "Player.h"
#include "LightSystem.h"
#include "AudioManager.h"
#include "Door.h"
#include "Main.h"
#include "ItemSystem.h"
#include "HUD.h"
#include "LeverPuzzle.h"

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
// FRUSTUM CULLING
// ==================================================
struct Frustum {
    glm::vec4 planes[6]; // left, right, bottom, top, near, far
};

Frustum ExtractFrustum(const glm::mat4& projView) {
    Frustum frustum;
    frustum.planes[0] = glm::vec4(
        projView[0][3] + projView[0][0],
        projView[1][3] + projView[1][0],
        projView[2][3] + projView[2][0],
        projView[3][3] + projView[3][0]
    );
    frustum.planes[1] = glm::vec4(
        projView[0][3] - projView[0][0],
        projView[1][3] - projView[1][0],
        projView[2][3] - projView[2][0],
        projView[3][3] - projView[3][0]
    );
    frustum.planes[2] = glm::vec4(
        projView[0][3] + projView[0][1],
        projView[1][3] + projView[1][1],
        projView[2][3] + projView[2][1],
        projView[3][3] + projView[3][1]
    );
    frustum.planes[3] = glm::vec4(
        projView[0][3] - projView[0][1],
        projView[1][3] - projView[1][1],
        projView[2][3] - projView[2][1],
        projView[3][3] - projView[3][1]
    );
    frustum.planes[4] = glm::vec4(
        projView[0][2],
        projView[1][2],
        projView[2][2],
        projView[3][2]
    );
    frustum.planes[5] = glm::vec4(
        projView[0][3] - projView[0][2],
        projView[1][3] - projView[1][2],
        projView[2][3] - projView[2][2],
        projView[3][3] - projView[3][2]
    );

    for (int i = 0; i < 6; i++) {
        float length = glm::length(glm::vec3(frustum.planes[i]));
        frustum.planes[i] /= length;
    }
    return frustum;
}

bool IsAABBInFrustum(const Frustum& frustum, const glm::vec3& min, const glm::vec3& max) {
    std::array<glm::vec3, 8> corners = {
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(min.x, max.y, max.z),
        glm::vec3(max.x, max.y, max.z)
    };

    for (int i = 0; i < 6; i++) {
        int out = 0;
        for (int j = 0; j < 8; j++) {
            if (glm::dot(frustum.planes[i], glm::vec4(corners[j], 1.0f)) < 0.0f)
                out++;
        }
        if (out == 8)
            return false;
    }
    return true;
}

// Función para calcular AABB local usando los meshes públicos de Model
void CalculateModelAABB(Model* model, glm::vec3& outMin, glm::vec3& outMax) {
    outMin = glm::vec3(FLT_MAX);
    outMax = glm::vec3(-FLT_MAX);

    for (const auto& mesh : model->meshes) {
        for (const auto& vertex : mesh.vertices) {
            outMin = glm::min(outMin, vertex.Position);
            outMax = glm::max(outMax, vertex.Position);
        }
    }

    if (outMin.x > outMax.x) {
        outMin = glm::vec3(-0.5f);
        outMax = glm::vec3(0.5f);
    }
}

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

    char c;
    int num;
    std::vector<int> filaActual;

    while (archivo >> c)
    {
        if (c == '{')
        {
            filaActual.clear();
        }
        else if (c == '}')
        {
            if (!filaActual.empty())
                matriz.push_back(filaActual);
        }
        else if (isdigit((unsigned char)c))
        {
            archivo.putback(c);
            archivo >> num;
            filaActual.push_back(num);
        }
        else if (c == ',')
        {
            continue;
        }
    }

    return matriz;
}

float RandomRange(float min, float max)
{
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

glm::vec3 BlenderToOpenGL(float blenderX, float blenderY, float blenderZ)
{
    return glm::vec3(
        blenderX,
        blenderZ - 1.0f,
        -blenderY
    );
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

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    player.camera.ProcessMouseMovement(xpos - lastX, lastY - ypos);

    lastX = xpos;
    lastY = ypos;
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

    if (mPressed && !mPressedLastFrame)
        ToggleFullscreen(window);

    mPressedLastFrame = mPressed;
}

float GetAspectRatio()
{
    if (currentWindowHeight <= 0)
        return (float)SCR_WIDTH / (float)SCR_HEIGHT;

    return (float)currentWindowWidth / (float)currentWindowHeight;
}

glm::mat4 GetViewWithStaminaEffect(Player& player)
{
    float staminaPercent = player.GetStaminaPercent();

    float shakeIntensity = 0.0f;

    if (player.isExhausted)
        shakeIntensity = 0.045f;
    else if (staminaPercent <= STAMINA_LOW_PERCENT)
        shakeIntensity = 0.018f;

    if (shakeIntensity <= 0.0f)
        return player.camera.GetViewMatrix();

    float time = (float)glfwGetTime();
    float shakeX = sin(time * 35.0f) * shakeIntensity;
    float shakeY = cos(time * 28.0f) * shakeIntensity;

    glm::vec3 right = glm::normalize(glm::cross(player.camera.Front, player.camera.Up));
    glm::vec3 shakeOffset = right * shakeX + player.camera.Up * shakeY;
    glm::vec3 visualPos = player.camera.Position + shakeOffset;

    return glm::lookAt(visualPos, visualPos + player.camera.Front, player.camera.Up);
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
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    srand((unsigned int)time(NULL));

    // AUDIO
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
        audio.LoadSound("error", "Resources/Audio/error.mp3", false);
        audio.LoadSound("correct", "Resources/Audio/correct.mp3", false);
        audio.LoadSound("palanca_sonido", "Resources/Audio/palanca.mp3", false);
        audio.LoadSound("medio_cansado", "Resources/Audio/medio_cansado.mp3", false);
        audio.LoadSound("cansado_completo", "Resources/Audio/cansado_completo.mp3", false);

        audio.SetVolume("intro", 0.45f);
        audio.SetVolume("caminata", 0.35f);
        audio.SetVolume("correr", 0.45f);
        audio.SetVolume("abrir_puerta", 0.8f);
        audio.SetVolume("toque_puerta", 0.8f);
        audio.SetVolume("sonido_luces", 0.55f);
        audio.SetVolume("sonido_tension", 0.45f);
        audio.SetVolume("risa_tension", 0.35f);
        audio.SetVolume("recoger", 0.7f);
        audio.SetVolume("error", 0.8f);
        audio.SetVolume("correct", 0.8f);
        audio.SetVolume("palanca_sonido", 0.7f);
        audio.SetVolume("medio_cansado", 0.55f);
        audio.SetVolume("cansado_completo", 0.9f);

        nextTensionSoundTime = RandomRange(20.0f, 40.0f);
        nextLaughSoundTime = RandomRange(60.0f, 120.0f);
        nextDoorKnockTime = 40.0f;
    }

    // MATRIZ DE COLISIONES
    laberinto = cargarLaberinto("Resources/Models/Casa/matriz_tremenuwu.txt");
    if (laberinto.empty())
        std::cout << "ADVERTENCIA: Matriz vacia." << std::endl;
    else
        std::cout << "Matriz cargada (" << laberinto.size() << " filas)." << std::endl;

    // MONSTRUO — waypoints
    Pathfinding iaMonstruo;
    iaMonstruo.temperatura = 0.3f;

    std::vector<glm::ivec2> waypointsMonstruo;
    if (!laberinto.empty())
    {
        for (int f = 0; f < (int)laberinto.size(); ++f)
            for (int c = 0; c < (int)laberinto[f].size(); ++c)
                if (laberinto[f][c] == 2)
                    waypointsMonstruo.push_back(glm::ivec2(c, f));
    }

    std::vector<glm::ivec2> waypointsUnicos;
    if (!waypointsMonstruo.empty())
    {
        std::vector<bool> usado(waypointsMonstruo.size(), false);
        for (size_t i = 0; i < waypointsMonstruo.size(); ++i)
        {
            if (usado[i]) continue;
            waypointsUnicos.push_back(waypointsMonstruo[i]);
            for (size_t j = i + 1; j < waypointsMonstruo.size(); ++j)
            {
                if (!usado[j] && glm::length(glm::vec2(waypointsMonstruo[j] - waypointsMonstruo[i])) < 2.0f)
                    usado[j] = true;
            }
        }
    }

    glm::vec3 monsterPos(129.0f, MONSTER_HEIGHT, -98.0f);
    std::vector<glm::vec3> rutaSuaveMundo;
    size_t indiceRutaActual = 0;

    if (!laberinto.empty() && !waypointsUnicos.empty())
    {
        glm::ivec2 destino = waypointsUnicos[rand() % waypointsUnicos.size()];
        int sC = std::max(0, std::min((int)floor((monsterPos.x - OFFSET_X) / TAMANO_BLOQUE), (int)laberinto[0].size() - 1));
        int sF = std::max(0, std::min((int)floor((-monsterPos.z - OFFSET_Z) / TAMANO_BLOQUE), (int)laberinto.size() - 1));
        auto cruda = iaMonstruo.PlanificarRuta(glm::ivec2(sC, sF), destino, laberinto, iaMonstruo.temperatura);
        auto suave = iaMonstruo.SuavizarCamino(cruda, laberinto);
        for (auto& p : suave)
            rutaSuaveMundo.push_back(glm::vec3(OFFSET_X + p.x * TAMANO_BLOQUE + CENTRO_BLOQUE, MONSTER_HEIGHT, -(OFFSET_Z + p.y * TAMANO_BLOQUE + CENTRO_BLOQUE)));
    }

    // SHADERS
    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    Shader lampShader("shaders/lamp.vert", "shaders/lamp.frag");
    Shader depthShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");

    // MENU + carga de modelos
    Menu menu(SCR_WIDTH, SCR_HEIGHT);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!audio.IsPlaying("intro")) audio.Play("intro");

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

        if (fakeProgress < 0.9f) {
            fakeProgress += dt * 0.30f;
            if (fakeProgress > 0.9f) fakeProgress = 0.9f;
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

    // Precalcular AABB de los modelos principales
    glm::vec3 houseLocalMin, houseLocalMax;
    CalculateModelAABB(model, houseLocalMin, houseLocalMax);
    glm::vec3 monsterLocalMin, monsterLocalMax;
    CalculateModelAABB(monsterModel, monsterLocalMin, monsterLocalMax);

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
        audio.Shutdown();
        glfwTerminate();
        return 0;
    }

    if (audio.IsPlaying("intro")) audio.Stop("intro");

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    firstMouse = true;
    player.camera.MouseSensitivity = menu.mouseSensitivity;
    lastFrame = (float)glfwGetTime();

    // LUCES
    LightSystem lightSystem;

    // PUZZLE DE PALANCAS
    LeverPuzzle leverPuzzle;
    leverPuzzle.Init();

    // PUERTAS NORMALES
    std::vector<std::unique_ptr<Door>> doors;

    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_1.obj", glm::vec3(29.264f, 0.0f, 3.9708f),
        glm::vec3(28.2658f, 1.6f, 2.04128f), 0.0f, 90.0f, 120.0f, 4.0f, true));
    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_2.obj", glm::vec3(-18.673f, 0.0f, 3.9345f),
        glm::vec3(-18.673f, 1.6f, 3.9345f), 0.0f, 90.0f, 120.0f, 6.0f, true));
    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_3.obj", glm::vec3(-46.39f, 0.0f, -89.064f),
        glm::vec3(-46.39f, 1.6f, -89.064f), 0.0f, -90.0f, 120.0f, 6.0f, false));
    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_4.obj", glm::vec3(33.317f, 0.0f, -84.054f),
        glm::vec3(33.317f, 1.6f, -84.054f), 0.0f, 90.0f, 120.0f, 6.0f, false));
    doors.push_back(std::make_unique<Door>(
        "Resources/Models/Casa/puerta_5.obj", glm::vec3(53.266f, 0.0f, -107.91f),
        glm::vec3(53.266f, 1.6f, -107.91f), 0.0f, 90.0f, 120.0f, 6.0f, true));

    // AABB de las puertas normales
    std::vector<glm::vec3> doorAABBMin, doorAABBMax;
    for (auto& d : doors) {
        glm::vec3 pos = d->GetPosition();
        doorAABBMin.push_back(pos - glm::vec3(0.75f, 0.0f, 0.15f));
        doorAABBMax.push_back(pos + glm::vec3(0.75f, 3.5f, 0.15f));
    }

    // ==================================================
    // PUERTA FINAL (doble hoja) - NUEVO
    // ==================================================
    bool puzzleCompleted = false;
    // Ajusta estas coordenadas con las reales de tu escena
    glm::vec3 salida_hinge1(-10.0f, 0.0f, -20.0f);
    glm::vec3 salida_hinge2(-10.0f, 0.0f, -21.0f);
    glm::vec3 salida_interact(-10.0f, 1.6f, -20.5f);
    bool salidaOpenState = false;

    auto puerta_salida1 = std::make_unique<Door>(
        "Resources/Models/Casa/puerta_salida1.obj",
        salida_hinge1, salida_interact,
        0.0f, -90.0f, 120.0f, 4.0f, false, 1.2f,
        &puzzleCompleted, &salidaOpenState
    );
    auto puerta_salida2 = std::make_unique<Door>(
        "Resources/Models/Casa/puerta_salida2.obj",
        salida_hinge2, salida_interact,
        0.0f, 90.0f, 120.0f, 4.0f, false, 1.2f,
        &puzzleCompleted, &salidaOpenState
    );

    doors.push_back(std::move(puerta_salida1));
    doors.push_back(std::move(puerta_salida2));

    // Añadir sus AABB (mismo tamaño que las otras)
    for (size_t i = doors.size() - 2; i < doors.size(); ++i) {
        glm::vec3 pos = doors[i]->GetPosition();
        doorAABBMin.push_back(pos - glm::vec3(0.75f, 0.0f, 0.15f));
        doorAABBMax.push_back(pos + glm::vec3(0.75f, 3.5f, 0.15f));
    }

    // ITEMS Y LLAVES
    ItemSystem itemSystem;

    std::vector<glm::vec3> key1PiecePositions;
    key1PiecePositions.push_back(glm::vec3(-2.73801f, 0.35f, -108.702f));
    key1PiecePositions.push_back(glm::vec3(38.8061f, 0.35f, -79.4784f));
    key1PiecePositions.push_back(glm::vec3(-53.2314f, 0.35f, -42.2163f));
    itemSystem.SpawnKey1PiecesFixedPositions(key1PiecePositions);

    const bool DRAW_KEY2_MODEL = true;
    const bool DRAW_KEY3_MODEL = true;
    float key2BlenderX = 31.657f, key2BlenderY = -22.245f, key2BlenderZ = 2.227f;
    float key3BlenderX = -18.547f, key3BlenderY = -21.498f, key3BlenderZ = 1.031f;

    glm::vec3 key2Position = BlenderToOpenGL(key2BlenderX, key2BlenderY, key2BlenderZ);
    glm::vec3 key3Position = BlenderToOpenGL(key3BlenderX, key3BlenderY, key3BlenderZ);

    itemSystem.SpawnKey2At(key2Position, DRAW_KEY2_MODEL);
    itemSystem.SpawnKey3At(key3Position, DRAW_KEY3_MODEL);

    for (auto& item : itemSystem.items)
        std::cout << "[DEBUG KEY] pos=(" << item.position.x << ", " << item.position.y << ", " << item.position.z << ") visible=" << item.visible << std::endl;

    // HUD
    HUD hud;
    if (!hud.Init("Resources/Fonts/arial.ttf", currentWindowWidth, currentWindowHeight))
        std::cout << "[ADVERTENCIA] HUD sin fuente." << std::endl;

    float keyPickedTimer = 0.0f, key2PickedTimer = 0.0f, key3PickedTimer = 0.0f;

    // SHADOW MAPS
    unsigned int flashDepthMapFBO, lampDepthMapFBO;
    unsigned int flashDepthMap, lampDepthMap;
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    auto makeShadowMap = [&](unsigned int& fbo, unsigned int& tex) {
        glGenFramebuffers(1, &fbo); glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);
        glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        };
    makeShadowMap(flashDepthMapFBO, flashDepthMap);
    makeShadowMap(lampDepthMapFBO, lampDepthMap);

    // CUBOS PARA LÁMPARAS Y MARKERS
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

    float markerCubeVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f
    };

    unsigned int lampVAO, lampVBO;
    glGenVertexArrays(1, &lampVAO); glGenBuffers(1, &lampVBO);
    glBindVertexArray(lampVAO); glBindBuffer(GL_ARRAY_BUFFER, lampVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int markerVAO, markerVBO;
    glGenVertexArrays(1, &markerVAO); glGenBuffers(1, &markerVBO);
    glBindVertexArray(markerVAO); glBindBuffer(GL_ARRAY_BUFFER, markerVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(markerCubeVertices), markerCubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ==================================================
    // GAME LOOP
    // ==================================================
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        CheckFullscreenKey(window);

        // Jugador
        glm::vec3 oldPos = player.camera.Position;
        player.ProcessInput(window, deltaTime);
        player.UpdatePhysics(deltaTime);
        player.UpdateFlashlight();

        if (player.softBreathEvent) audio.Play("medio_cansado");
        if (player.hardBreathEvent) audio.Play("cansado_completo");

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

        // Monstruo
        glm::vec3 oldMonsterPos = monsterPos;
        if (!rutaSuaveMundo.empty() && indiceRutaActual < rutaSuaveMundo.size())
        {
            glm::vec3 dir = rutaSuaveMundo[indiceRutaActual] - monsterPos;
            float dist = glm::length(dir);
            if (dist > 0.1f) monsterPos += glm::normalize(dir) * MONSTER_SPEED * deltaTime;
            else indiceRutaActual++;
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
                rutaSuaveMundo.push_back(glm::vec3(OFFSET_X + p.x * TAMANO_BLOQUE + CENTRO_BLOQUE, MONSTER_HEIGHT, -(OFFSET_Z + p.y * TAMANO_BLOQUE + CENTRO_BLOQUE)));
            indiceRutaActual = 0;
        }

        if (!laberinto.empty())
        {
            float mg = 0.08f;
            float bz = -monsterPos.z;
            int fMin = (int)floor(((bz - mg) - OFFSET_Z) / TAMANO_BLOQUE);
            int fMax = (int)floor(((bz + mg) - OFFSET_Z) / TAMANO_BLOQUE);
            int cIzq = (int)floor(((monsterPos.x - mg) - OFFSET_X) / TAMANO_BLOQUE);
            int cDer = (int)floor(((monsterPos.x + mg) - OFFSET_X) / TAMANO_BLOQUE);
            bool col = fMin < 0 || fMax >= (int)laberinto.size() || cIzq < 0 || cDer >= (int)laberinto[0].size() ||
                laberinto[fMin][cIzq] == 1 || laberinto[fMin][cDer] == 1 || laberinto[fMax][cIzq] == 1 || laberinto[fMax][cDer] == 1;
            bool cerca = false;
            if (!rutaSuaveMundo.empty() && indiceRutaActual < rutaSuaveMundo.size() &&
                glm::length(monsterPos - rutaSuaveMundo[indiceRutaActual]) < 0.5f)
            {
                auto pt = rutaSuaveMundo[indiceRutaActual];
                int pc = (int)floor((pt.x - OFFSET_X) / TAMANO_BLOQUE);
                int pf = (int)floor((-pt.z - OFFSET_Z) / TAMANO_BLOQUE);
                if (pf >= 0 && pf < (int)laberinto.size() && pc >= 0 && pc < (int)laberinto[0].size() && laberinto[pf][pc] == 0)
                    cerca = true;
            }
            if (col && !cerca) monsterPos = oldMonsterPos;
        }

        // Audio movimiento
        bool moving = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        bool running = moving && shiftPressed && !player.isExhausted && player.stamina > 0.0f;

        if (running) {
            if (audio.IsPlaying("caminata")) audio.Stop("caminata");
            if (!audio.IsPlaying("correr")) audio.Play("correr");
        }
        else if (moving) {
            if (audio.IsPlaying("correr")) audio.Stop("correr");
            if (!audio.IsPlaying("caminata")) audio.Play("caminata");
        }
        else {
            if (audio.IsPlaying("caminata")) audio.Stop("caminata");
            if (audio.IsPlaying("correr")) audio.Stop("correr");
        }

        // Interacción
        bool ePressed = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
        bool ePressedOnce = ePressed && !ePressedLastFrame;

        bool hadKeyBefore = itemSystem.hasKey;
        bool hadKey2Before = itemSystem.hasKey2;
        bool hadKey3Before = itemSystem.hasKey3;

        itemSystem.Update(deltaTime, player.camera.Position, player.camera.Front, ePressed, ePressedLastFrame, &audio);

        if (!hadKeyBefore && itemSystem.hasKey) keyPickedTimer = 3.0f;
        if (!hadKey2Before && itemSystem.hasKey2) key2PickedTimer = 3.0f;
        if (!hadKey3Before && itemSystem.hasKey3) key3PickedTimer = 3.0f;

        if (keyPickedTimer > 0.0f) keyPickedTimer -= deltaTime;
        if (key2PickedTimer > 0.0f) key2PickedTimer -= deltaTime;
        if (key3PickedTimer > 0.0f) key3PickedTimer -= deltaTime;

        // Actualizar puzzle
        puzzleCompleted = leverPuzzle.IsComplete();

        // Actualizar puertas
        for (size_t i = 0; i < doors.size(); ++i)
        {
            bool keyForThisDoor = false;
            if (i == 0) keyForThisDoor = itemSystem.hasKey;
            else if (i == 1) keyForThisDoor = itemSystem.hasKey2;
            else if (i == 4) keyForThisDoor = itemSystem.hasKey3;
            // Puertas 5 y 6 (puzzle) no necesitan llave, su desbloqueo se controla externamente
            doors[i]->Update(deltaTime, player.camera.Position, player.camera.Front, ePressed, ePressedLastFrame, &audio, keyForThisDoor);
        }

        leverPuzzle.Update(player.camera.Position, ePressedOnce, deltaTime, &audio);
        ePressedLastFrame = ePressed;

        // HUD state
        HUDState hudState{};
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

        // Detectar item cercano
        for (auto& item : itemSystem.items)
        {
            if (!item.visible) continue;
            glm::vec2 playerXZ(player.camera.Position.x, player.camera.Position.z);
            glm::vec2 itemXZ(item.position.x, item.position.z);
            if (glm::length(playerXZ - itemXZ) < item.pickupRadius) {
                hudState.lookingAtItem = true;
                break;
            }
        }

        // Detectar puerta mirada
        for (size_t i = 0; i < doors.size(); ++i)
        {
            if (doors[i]->isBeingLookedAt)
            {
                if (i >= 5) // Puertas del puzzle
                {
                    hudState.lookingAtPuzzleDoor = true;
                    hudState.puzzleDoorBlocked = !puzzleCompleted;
                    hudState.doorIsOpen = doors[i]->IsOpen();
                    hudState.lookingAtDoor = false;
                }
                else
                {
                    hudState.lookingAtDoor = true;
                    hudState.doorIsOpen = doors[i]->IsOpen();
                    hudState.doorRequiresKey = false;
                    hudState.doorRequiresKey2 = false;
                    hudState.doorRequiresKey3 = false;
                    if (i == 0)      hudState.doorRequiresKey = doors[i]->RequiresKey();
                    else if (i == 1) hudState.doorRequiresKey2 = doors[i]->RequiresKey();
                    else if (i == 4) hudState.doorRequiresKey3 = doors[i]->RequiresKey();
                }
                break;
            }
        }

        // Eventos de tensión
        if (currentFrame >= nextTensionSoundTime) {
            audio.Play("sonido_tension");
            nextTensionSoundTime = currentFrame + RandomRange(30.0f, 70.0f);
        }
        if (currentFrame >= nextLaughSoundTime) {
            audio.Play("risa_tension");
            nextLaughSoundTime = currentFrame + RandomRange(90.0f, 180.0f);
        }
        if (currentFrame >= nextDoorKnockTime) {
            audio.Play("toque_puerta");
            nextDoorKnockTime = currentFrame + 40.0f;
        }

        // Luces
        lightSystem.Update(deltaTime, currentFrame, player.camera.Position);
        if (lightSystem.lightsFlickering) {
            if (!audio.IsPlaying("sonido_luces")) audio.Play("sonido_luces");
        }
        else {
            if (audio.IsPlaying("sonido_luces")) audio.Stop("sonido_luces");
        }

        // Matrices de sombra
        glm::mat4 flashLightSpaceMatrix =
            glm::perspective(glm::radians(55.0f), 1.0f, 0.05f, 50.0f) *
            glm::lookAt(player.flashlight.position, player.flashlight.position + player.flashlight.direction, glm::vec3(0, 1, 0));
        glm::vec3 slp = lightSystem.lampPositions[26];
        glm::mat4 lampLightSpaceMatrix =
            glm::perspective(glm::radians(110.0f), 1.0f, 0.1f, 38.0f) *
            glm::lookAt(slp, slp + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));

        // Depth pass linterna
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, flashDepthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthShader.use();
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(flashLightSpaceMatrix));
        glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
        model->Draw(depthShader.ID);
        for (auto& d : doors) d->Draw(depthShader.ID, modelMat);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Depth pass lámpara 26
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, lampDepthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthShader.use();
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lampLightSpaceMatrix));
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
        model->Draw(depthShader.ID);
        for (auto& d : doors) d->Draw(depthShader.ID, modelMat);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // RENDER NORMAL
        glViewport(0, 0, currentWindowWidth, currentWindowHeight);
        glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = GetViewWithStaminaEffect(player);
        float currentFov = player.camera.Zoom;
        if (player.isExhausted) currentFov = 52.0f;
        else if (player.GetStaminaPercent() <= STAMINA_LOW_PERCENT) currentFov = 48.0f;
        glm::mat4 projection = glm::perspective(glm::radians(currentFov), GetAspectRatio(), 0.1f, 1000.0f);

        // Frustum actual
        glm::mat4 projView = projection * view;
        Frustum frustum = ExtractFrustum(projView);

        shader.use();
        // Configurar luces y sombras...
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "flashLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(flashLightSpaceMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "lampLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lampLightSpaceMatrix));
        glUniform1i(glGetUniformLocation(shader.ID, "lampShadowEnabled"), 1);
        glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, flashDepthMap); glUniform1i(glGetUniformLocation(shader.ID, "flashShadowMap"), 10);
        glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_2D, lampDepthMap); glUniform1i(glGetUniformLocation(shader.ID, "lampShadowMap"), 11);
        glUniform1i(glGetUniformLocation(shader.ID, "flashlightOn"), player.flashlight.on);
        glUniform3f(glGetUniformLocation(shader.ID, "flashlightPos"), player.flashlight.position.x, player.flashlight.position.y, player.flashlight.position.z);
        glUniform3f(glGetUniformLocation(shader.ID, "flashlightDir"), player.flashlight.direction.x, player.flashlight.direction.y, player.flashlight.direction.z);

        for (int i = 0; i < NUM_LAMPS; i++) {
            std::string b = "lights[" + std::to_string(i) + "]";
            float lin = 0.045f, quad = 0.014f, pwr = lightSystem.intensities[i] * 0.75f, range = 13.0f;
            if (i == 2 || i == 3 || i == 4) { lin = 0.09f; quad = 0.032f; pwr = lightSystem.intensities[i] * 0.65f; range = 9.0f; }
            if (i == 29 || i == 34) { lin = 0.018f; quad = 0.0035f; pwr = lightSystem.intensities[i] * 1.5f; range = 24.0f; }
            if (i == 26) { lin = 0.026f; quad = 0.006f; pwr = lightSystem.intensities[i] * 1.25f; range = 28.0f; }
            glUniform3f(glGetUniformLocation(shader.ID, (b + ".position").c_str()), lightSystem.lampPositions[i].x, lightSystem.lampPositions[i].y, lightSystem.lampPositions[i].z);
            glUniform3f(glGetUniformLocation(shader.ID, (b + ".color").c_str()), 1.0f, 0.95f, 0.8f);
            glUniform1f(glGetUniformLocation(shader.ID, (b + ".intensity").c_str()), pwr);
            glUniform1f(glGetUniformLocation(shader.ID, (b + ".constant").c_str()), 1.0f);
            glUniform1f(glGetUniformLocation(shader.ID, (b + ".linear").c_str()), lin);
            glUniform1f(glGetUniformLocation(shader.ID, (b + ".quadratic").c_str()), quad);
            glUniform1f(glGetUniformLocation(shader.ID, (b + ".range").c_str()), range);
        }

        glUniform3f(glGetUniformLocation(shader.ID, "viewPos"), player.camera.Position.x, player.camera.Position.y, player.camera.Position.z);
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(shader.ID, "highlightColor"), 0.0f, 0.0f, 0.0f);

        // Casa (con culling)
        glm::vec3 houseWorldMin = houseLocalMin + glm::vec3(0.0f, -1.0f, 0.0f);
        glm::vec3 houseWorldMax = houseLocalMax + glm::vec3(0.0f, -1.0f, 0.0f);
        if (IsAABBInFrustum(frustum, houseWorldMin, houseWorldMax)) {
            glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
            model->Draw(shader.ID);
        }

        // Puertas (con culling)
        for (size_t i = 0; i < doors.size(); ++i) {
            if (IsAABBInFrustum(frustum, doorAABBMin[i], doorAABBMax[i])) {
                doors[i]->Draw(shader.ID, modelMat);
            }
        }

        // Palancas (siempre se dibujan)
        leverPuzzle.Draw(shader, modelMat);

        // Monstruo
        lampShader.use();
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 mm = glm::scale(glm::translate(glm::mat4(1.0f), monsterPos), glm::vec3(0.12f));
        glm::vec3 monsterWorldMin = monsterPos + monsterLocalMin * 0.12f;
        glm::vec3 monsterWorldMax = monsterPos + monsterLocalMax * 0.12f;
        if (IsAABBInFrustum(frustum, monsterWorldMin, monsterWorldMax)) {
            glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(mm));
            glUniform3f(glGetUniformLocation(lampShader.ID, "lampColor"), 1.0f, 0.2f, 0.2f);
            monsterModel->Draw(lampShader.ID);
        }

        // Lámparas (con culling)
        glBindVertexArray(lampVAO);
        for (int i = 0; i < NUM_LAMPS; i++) {
            if (!lightSystem.lampEnabled[i]) continue;
            glm::vec3 lampMin = lightSystem.lampPositions[i] - glm::vec3(0.5f, 0.05f, 0.2f);
            glm::vec3 lampMax = lightSystem.lampPositions[i] + glm::vec3(0.5f, 0.05f, 0.2f);
            if (!IsAABBInFrustum(frustum, lampMin, lampMax)) continue;

            glm::mat4 lm = glm::translate(glm::mat4(1.0f), lightSystem.lampPositions[i]);
            glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lm));
            float br = lightSystem.intensities[i];
            glUniform3f(glGetUniformLocation(lampShader.ID, "lampColor"), br, 0.95f * br, 0.8f * br);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Markers y modelos de llaves
        itemSystem.DrawGlowMarkers(lampShader.ID, view, projection, markerVAO);
        itemSystem.Draw(shader.ID, view, projection, markerVAO, modelMat);

        // HUD
        hud.Resize(currentWindowWidth, currentWindowHeight);
        hud.Render(hudState);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpieza
    delete model; delete monsterModel;
    glDeleteVertexArrays(1, &lampVAO); glDeleteBuffers(1, &lampVBO);
    glDeleteVertexArrays(1, &markerVAO); glDeleteBuffers(1, &markerVBO);
    glDeleteFramebuffers(1, &flashDepthMapFBO); glDeleteFramebuffers(1, &lampDepthMapFBO);
    glDeleteTextures(1, &flashDepthMap); glDeleteTextures(1, &lampDepthMap);
    audio.Shutdown();
    glfwTerminate();

    return 0;
}