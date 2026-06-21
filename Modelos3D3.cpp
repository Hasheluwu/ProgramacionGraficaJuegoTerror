#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "AnimatedModel.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <memory>
#include <map>
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
#include "Switch.h"
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
bool  tPressedLastFrame = false;   // <-- Tecla T para ropero

float nextTensionSoundTime = 0.0f;
float nextLaughSoundTime = 0.0f;
float nextDoorKnockTime = 0.0f;
float nextRisaExtraTime = 0.0f;
float nextCorridaTime = 0.0f;

float escapeTimer = 0.0f;
int   musicaFondoActual = -1;

bool pPressedLastFrame = false;

// Junto a las otras variables bool...LastFrame, cerca del inicio:
bool oPressedLastFrame = false;

// ==========================================
// CONFIGURACIÓN DE LA MATRIZ DE COLISIONES
// ==========================================
const float OFFSET_X = -76.40f;
const float OFFSET_Z = -24.01f;      // actualizado según Blender
const float TAMANO_BLOQUE = 0.4f;
const float CENTRO_BLOQUE = TAMANO_BLOQUE / 2.0f;
const float NIVEL_DEL_SUELO = 2.27f;

// ==================== CONFIGURACIÓN DEL MONSTRUO ====================
const float MONSTER_HEIGHT = -0.5f;
const float MONSTER_SPEED = 2.5f;
const float MONSTER_RADIO_ACEPTACION = 1.0f;
const float MONSTER_DIR_LERP = 1.0f;

const glm::vec3 MONSTER_COLOR = glm::vec3(1.0f, 1.0f, 1.0f);

const int ANIM_IDLE = 0;
const int ANIM_WALK = 1;
const int ANIM_RUN = 2;
const int ANIM_ATTACK = 3;

const float MONSTER_VISION_RANGE = 20.0f;
const float MONSTER_VISION_ANGLE = 90.0f;
const int   MONSTER_VISION_RAYS = 7;
const float MONSTER_LIGHT_THRESHOLD = 0.15f;

const float PROB_IR_A_SWITCH = 1.0f;

enum class MonsterState { PATROL, PURSUIT, ATTACK, ATTACK_COOLDOWN };
MonsterState monsterState = MonsterState::PATROL;

const float PURSUIT_SPEED = 4.0f;
const float DETECTION_TIME_REQUIRED = 1.0f;
const float PURSUIT_TIMEOUT = 3.0f;
const float ATTACK_RANGE = 2.0f;
const float ATTACK_RANGE_TIME = 0.0f;
const float ATTACK_IDLE_TIME = 2.0f;
const float ATTACK_COOLDOWN_TIME = 5.0f;
const float AWARENESS_RADIUS = 5.0f;

const glm::vec3 MONSTER_LOCAL_AABB_MIN = glm::vec3(-50.0f, 0.0f, -50.0f);
const glm::vec3 MONSTER_LOCAL_AABB_MAX = glm::vec3(50.0f, 200.0f, 50.0f);
const float MONSTER_DRAW_SCALE = 0.02f;

std::vector<std::vector<int>> laberinto;

struct IVec2Cmp {
    bool operator()(const glm::ivec2& a, const glm::ivec2& b) const {
        if (a.x != b.x) return a.x < b.x;
        return a.y < b.y;
    }
};


int playerLives = 3;


// ==================================================
// FRUSTUM CULLING
// ==================================================
struct Frustum {
    glm::vec4 planes[6];
};

// ==================================================
// ROPERO
// ==================================================
struct RoperoData {
    glm::mat4 modelMatrix;
    glm::vec3 worldPosition;
    std::unique_ptr<Door> puertaIzq;
    std::unique_ptr<Door> puertaDer;
    std::shared_ptr<bool> openState;
};
std::vector<RoperoData> roperosInstanciados;
Model* roperoCuerpoModel = nullptr;

bool        playerHidden = false;
RoperoData* currentHideRopero = nullptr;
float       playerHideYaw = 0.0f;
float       playerHidePitch = 0.0f;

// ==================================================
// DEBUG VISIÓN MONSTRUO
// ==================================================
bool debugVisionActive = false;
bool lPressedLastFrame = false;
unsigned int debugLineVAO, debugLineVBO;

// Mapa de celdas de puerta (valor 4) a índice en el vector doors
std::map<glm::ivec2, int, IVec2Cmp> celdaAPuertaIdx;


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
// Conversión mundo <-> matriz
// ==================================================
glm::vec3 CeldaAMundo(glm::ivec2 celda, float altura)
{
    float px = OFFSET_X + (celda.x * TAMANO_BLOQUE) + CENTRO_BLOQUE;
    float pz = -(OFFSET_Z + (celda.y * TAMANO_BLOQUE) + CENTRO_BLOQUE);
    return glm::vec3(px, altura, pz);
}

glm::ivec2 MundoACelda(glm::vec3 pos)
{
    int c = (int)floor((pos.x - OFFSET_X) / TAMANO_BLOQUE);
    int f = (int)floor((-pos.z - OFFSET_Z) / TAMANO_BLOQUE);
    return glm::ivec2(c, f);
}




bool ColisionConPuertas(const glm::vec3& pos, float radio,
    const std::vector<std::unique_ptr<Door>>& doors,
    const std::map<glm::ivec2, int, IVec2Cmp>& celdaAPuertaIdx)
{
    glm::ivec2 centro = MundoACelda(pos);
    // Convertir el radio a número de celdas (redondeando hacia arriba) + 2 de margen
    int radioCeldas = (int)ceil(radio / TAMANO_BLOQUE) + 2;
    for (int df = -radioCeldas; df <= radioCeldas; ++df) {
        for (int dc = -radioCeldas; dc <= radioCeldas; ++dc) {
            glm::ivec2 celda = centro + glm::ivec2(dc, df);
            auto it = celdaAPuertaIdx.find(celda);
            if (it != celdaAPuertaIdx.end()) {
                int idx = it->second;
                if (idx >= 0 && idx < (int)doors.size() && !doors[idx]->IsOpen()) {
                    return true;
                }
            }
        }
    }
    return false;
}

// ==================================================
// Detección del jugador
// ==================================================
float CalcularLuzJugador(const glm::vec3& pos, LightSystem& ls, bool flashlightOn)
{
    float luzTotal = 0.0f;
    for (int i = 0; i < NUM_LAMPS; i++) {
        if (!ls.lampEnabled[i]) continue;
        float dist = glm::length(pos - ls.lampPositions[i]);
        if (dist < 0.001f) dist = 0.001f;
        float atenuacion = 1.0f / (1.0f + 0.045f * dist + 0.014f * dist * dist);
        luzTotal += ls.intensities[i] * atenuacion;
    }
    if (flashlightOn) luzTotal += 0.5f;
    return glm::clamp(luzTotal, 0.0f, 1.0f);
}

bool RaycastMatriz(const glm::vec3& origen, const glm::vec3& destino,
    const vector<vector<int>>& mapa)
{
    int pasos = 30;
    for (int i = 1; i <= pasos; i++) {
        float t = (float)i / pasos;
        glm::vec3 p = origen + (destino - origen) * t;
        glm::ivec2 celda = MundoACelda(p);
        int filas = (int)mapa.size();
        int columnas = (int)mapa[0].size();
        if (celda.y < 0 || celda.y >= filas || celda.x < 0 || celda.x >= columnas)
            return false;
        if (mapa[celda.y][celda.x] == 1)
            return false;
    }
    return true;
}

bool MonstruoDetectaJugador(
    const glm::vec3& monsterPos, const glm::vec3& monsterDir,
    const glm::vec3& playerPos, float luzJugador,
    const vector<vector<int>>& mapa,
    float rangoMax, float anguloMaxGrados, int numRayos, float umbralLuz)
{

    float rangoEfectivo = rangoMax * glm::clamp((luzJugador - 0.4f) / 0.1f, 0.5f, 1.0f);

    glm::vec3 diff = playerPos - monsterPos;
    float distancia = glm::length(glm::vec2(diff.x, diff.z));
    if (distancia > rangoEfectivo) return false;

    glm::vec2 dirFlat = glm::normalize(glm::vec2(monsterDir.x, monsterDir.z));
    glm::vec2 diffFlat = glm::normalize(glm::vec2(diff.x, diff.z));
    float cosAngulo = glm::dot(dirFlat, diffFlat);
    float anguloRad = glm::radians(anguloMaxGrados);
    if (cosAngulo < cos(anguloRad)) return false;

    if (luzJugador < umbralLuz) return false;

    float spread = glm::radians(anguloMaxGrados);
    for (int r = 0; r < numRayos; r++) {
        float t = (numRayos == 1) ? 0.0f : -1.0f + 2.0f * r / (numRayos - 1);
        float offsetAngulo = t * spread * 0.5f;
        float cosO = cos(offsetAngulo), sinO = sin(offsetAngulo);
        glm::vec2 rayDir = glm::vec2(
            dirFlat.x * cosO - dirFlat.y * sinO,
            dirFlat.x * sinO + dirFlat.y * cosO);
        glm::vec3 puntoRayo = monsterPos + glm::vec3(rayDir.x, 0, rayDir.y) * distancia;
        if (r == numRayos / 2) puntoRayo = playerPos;
        if (RaycastMatriz(monsterPos, puntoRayo, mapa))
            return true;
    }
    return false;
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
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { glfwTerminate(); return -1; }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    srand((unsigned int)time(NULL));

    // ==================================================
    // AUDIO
    // ==================================================
    if (!audio.Init())
        std::cout << "ERROR: No se pudo iniciar el sistema de audio." << std::endl;
    else
    {
        // --- Sonidos 2D (ambiente, jugador, UI) ---
        audio.LoadSound("intro", "Resources/Audio/intro.mp3", true);
        audio.LoadSound("caminata", "Resources/Audio/caminata.mp3", true);
        audio.LoadSound("correr", "Resources/Audio/correr.mp3", true);

        // Sonido de puerta 2D (con volumen ajustado manualmente por distancia)
        audio.LoadSound("abrir_puerta", "Resources/Audio/abrir_puerta.wav", false);
        // El sonido de toque_puerta se mantiene 2D porque es ambiente genérico
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

        audio.LoadSound("musica_fondo_1", "Resources/Audio/musica_fondo_1.mp3", true);
        audio.LoadSound("musica_fondo_2", "Resources/Audio/musica_fondo_2.mp3", true);
        audio.SetVolume("musica_fondo_1", 0.25f);
        audio.SetVolume("musica_fondo_2", 0.25f);

        audio.LoadSound("musica_escape", "Resources/Audio/musica_escape.mp3", true);
        audio.SetVolume("musica_escape", 0.4f);

        // "monstruo_risa" ya no se utiliza; en su lugar usamos "risa_fondo_extra"
        // audio.LoadSound("monstruo_risa", "Resources/Audio/monstruo_risa.mp3", false);
        audio.LoadSound("monstruo_alerta", "Resources/Audio/monstruo_alerta.mp3", false);
        // audio.SetVolume("monstruo_risa", 0.6f);
        audio.SetVolume("monstruo_alerta", 0.7f);

        audio.LoadSound("risa_fondo_extra", "Resources/Audio/risa_fondo_extra.mp3", false);
        audio.LoadSound("corrida_fondo", "Resources/Audio/corrida_fondo.mp3", false);
        audio.SetVolume("risa_fondo_extra", 0.4f);
        audio.SetVolume("corrida_fondo", 0.35f);

        // Sonidos del monstruo (simulamos 3D ajustando volumen por distancia)
        audio.LoadSound("forcejeo", "Resources/Audio/forcejeo.mp3", false);

        audio.LoadSound("pasos_monstruo", "Resources/Audio/pasos_monstruo.mp3", true);
        audio.LoadSound("correr_monstruo", "Resources/Audio/correr_monstruo.mp3", true);

        audio.SetVolume("pasos_monstruo", 1.0f);
        audio.SetVolume("correr_monstruo", 1.0f);
        audio.SetVolume("forcejeo", 1.0f);

        audio.SetVolume("intro", 0.45f);
        audio.SetVolume("caminata", 0.35f);
        audio.SetVolume("correr", 0.45f);
        audio.SetVolume("abrir_puerta", 0.8f);
        audio.SetVolume("toque_puerta", 0.8f);
        audio.SetVolume("sonido_luces", 0.10f);
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
        nextRisaExtraTime = RandomRange(30.0f, 50.0f);
        nextCorridaTime = RandomRange(45.0f, 90.0f);
    }

    // ==================================================
    // MATRIZ DE COLISIONES
    // ==================================================
    laberinto = cargarLaberinto("Resources/Models/Casa/matriz_tremenpuertas4.txt");
    if (laberinto.empty())
        std::cout << "ADVERTENCIA: Matriz vacia." << std::endl;
    else
        std::cout << "Matriz cargada (" << laberinto.size() << " filas)." << std::endl;

    // ==================================================
    // MONSTRUO – inicialización
    // ==================================================
    Pathfinding iaMonstruo;
    iaMonstruo.temperatura = 0.3f;
    iaMonstruo.penalizacionPared = 5.0f;
    iaMonstruo.pesoOctile = 1.05f;

    std::vector<glm::ivec2> waypointsMonstruo;
    if (!laberinto.empty())
        for (int f = 0; f < (int)laberinto.size(); ++f)
            for (int c = 0; c < (int)laberinto[f].size(); ++c)
                if (laberinto[f][c] == 2)
                    waypointsMonstruo.push_back(glm::ivec2(c, f));

    std::vector<glm::ivec2> waypointsUnicos;
    {
        const float DIST_AGRUPACION = 2.0f;
        std::vector<bool> usado(waypointsMonstruo.size(), false);
        for (size_t i = 0; i < waypointsMonstruo.size(); ++i) {
            if (usado[i]) continue;
            waypointsUnicos.push_back(waypointsMonstruo[i]);
            for (size_t j = i + 1; j < waypointsMonstruo.size(); ++j) {
                if (usado[j]) continue;
                float dist = glm::length(glm::vec2(waypointsMonstruo[j] - waypointsMonstruo[i]));
                if (dist < DIST_AGRUPACION) usado[j] = true;
            }
        }
    }

    glm::vec3 monsterPos(129.0f, MONSTER_HEIGHT, -98.0f);
    glm::vec3 oldMonsterPos = monsterPos;
    glm::vec3 monsterDirActual(1.0f, 0.0f, 0.0f);

    float detectionTimer = 0.0f;
    float pursuitTimer = 0.0f;
    float attackRangeTimer = 0.0f;
    float attackIdleTimer = 0.0f;
    float cooldownTimer = 0.0f;
    bool  attackAnimDone = false;
    glm::vec3 lastKnownPlayerPos = glm::vec3(0.0f);

    std::vector<glm::vec3> rutaMundo;
    size_t indiceRuta = 0;
    int    waypointDestinoIdx = -1;

    glm::ivec2 monsterSwitchDestino = glm::ivec2(-1, -1);

    bool  forcejeando = false;
    float forcejeoTimer = 0.0f;
    Door* puertaForcejeando = nullptr;
    bool  sonidoForcejeoReproducido = false;

    float monsterStepTimer = 0.0f;

    if (!laberinto.empty()) {
        glm::ivec2 celdaInicio = MundoACelda(monsterPos);
        int filas = (int)laberinto.size();
        int columnas = (int)laberinto[0].size();
        if (celdaInicio.y < 0 || celdaInicio.y >= filas ||
            celdaInicio.x < 0 || celdaInicio.x >= columnas ||
            laberinto[celdaInicio.y][celdaInicio.x] == 1) {
            if (!waypointsUnicos.empty()) {
                glm::ivec2 wp = waypointsUnicos[rand() % waypointsUnicos.size()];
                monsterPos = CeldaAMundo(wp, MONSTER_HEIGHT);
                std::cout << "[IA] Posicion inicial invalida. Monstruo reposicionado en waypoint." << std::endl;
            }
        }
    }

    // ==================================================
    // SHADERS
    // ==================================================
    Shader shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    Shader lampShader("shaders/lamp.vert", "shaders/lamp.frag");
    Shader depthShader("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");
    Shader monsterShader("shaders/anim_vertex.glsl", "shaders/monster.frag");
    Shader debugLineShader("shaders/debug_line.vert", "shaders/debug_line.frag");

    // ==================================================
    // MENU + carga de modelos
    // ==================================================
    Menu menu(SCR_WIDTH, SCR_HEIGHT);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (!audio.IsPlaying("intro")) audio.Play("intro");

    menu.state = MenuState::LOADING;
    Model* model = nullptr;
    AnimatedModel monsterModel;

    float fakeProgress = 0.0f;
    float loadLastFrame = (float)glfwGetTime();
    bool  modelLoaded = false;

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
            model = new Model("Resources/Models/Casa/sotano2.obj");

            std::string carpeta = "Resources/Models/Monstruo/";
            monsterModel.LoadModel(carpeta + "Ch30_nonPBR.fbx");
            monsterModel.LoadAnimation(carpeta + "Idle.fbx", "idle");
            monsterModel.LoadAnimation(carpeta + "Sad Walk.fbx", "walk");
            monsterModel.LoadAnimation(carpeta + "Fast Run.fbx", "run");
            monsterModel.LoadAnimation(carpeta + "Surprise Uppercut.fbx", "attack");
            monsterModel.SetAnimation("idle");

            // Cargar modelo del cuerpo del ropero
            roperoCuerpoModel = new Model("Resources/Models/Casa/cuerpo_ropero.obj");

            modelLoaded = true;
            menu.SetLoadingProgress(1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            menu.Render();
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    // Precalcular AABB de la casa
    glm::vec3 houseLocalMin, houseLocalMax;
    CalculateModelAABB(model, houseLocalMin, houseLocalMax);

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
        delete model;
        delete roperoCuerpoModel;
        audio.Shutdown(); glfwTerminate(); return 0;
    }

    if (audio.IsPlaying("intro")) audio.Stop("intro");

    if (rand() % 2 == 0) { audio.Play("musica_fondo_1"); musicaFondoActual = 1; }
    else { audio.Play("musica_fondo_2"); musicaFondoActual = 2; }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    firstMouse = true;
    player.camera.MouseSensitivity = menu.mouseSensitivity;
    lastFrame = (float)glfwGetTime();

    // ==================================================
    // LUCES
    // ==================================================
    LightSystem lightSystem;

    // ==================================================
    // PUZZLE DE PALANCAS
    // ==================================================
    LeverPuzzle leverPuzzle;
    leverPuzzle.Init();

    // ==================================================
    // PUERTAS NORMALES
    // ==================================================
    std::vector<std::unique_ptr<Door>> doors;
    doors.push_back(std::make_unique<Door>("Resources/Models/Casa/puerta_1.obj", glm::vec3(29.264f, 0.0f, 3.9708f), glm::vec3(28.2658f, 1.6f, 2.04128f), 0.0f, 90.0f, 120.0f, 4.0f, true));
    doors.push_back(std::make_unique<Door>("Resources/Models/Casa/puerta_2.obj", glm::vec3(-18.673f, 0.0f, 3.9345f), glm::vec3(-18.673f, 1.6f, 3.9345f), 0.0f, 90.0f, 120.0f, 6.0f, true));
    doors.push_back(std::make_unique<Door>("Resources/Models/Casa/puerta_3.obj", glm::vec3(-46.39f, 0.0f, -89.064f), glm::vec3(-46.39f, 1.6f, -89.064f), 0.0f, -90.0f, 120.0f, 6.0f, false));
    doors.push_back(std::make_unique<Door>("Resources/Models/Casa/puerta_4.obj", glm::vec3(33.317f, 0.0f, -84.054f), glm::vec3(33.317f, 1.6f, -84.054f), 0.0f, 90.0f, 120.0f, 6.0f, false));
    doors.push_back(std::make_unique<Door>("Resources/Models/Casa/puerta_5.obj", glm::vec3(53.266f, 0.0f, -107.91f), glm::vec3(53.266f, 1.6f, -107.91f), 0.0f, 90.0f, 120.0f, 6.0f, true));

    std::vector<glm::vec3> doorAABBMin, doorAABBMax;
    for (auto& d : doors) {
        glm::vec3 pos = d->GetPosition();
        doorAABBMin.push_back(pos - glm::vec3(0.75f, 0.0f, 0.15f));
        doorAABBMax.push_back(pos + glm::vec3(0.75f, 3.5f, 0.15f));
    }

    // ==================================================
    // PUERTA FINAL (doble hoja)
    // ==================================================
    bool puzzleCompleted = false;
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

    for (size_t i = doors.size() - 2; i < doors.size(); ++i) {
        glm::vec3 pos = doors[i]->GetPosition();
        doorAABBMin.push_back(pos - glm::vec3(0.75f, 0.0f, 0.15f));
        doorAABBMax.push_back(pos + glm::vec3(0.75f, 3.5f, 0.15f));
    }

    // ==================================================
    // MAPA DE CELDAS DE PUERTA (valor 4 en la matriz)
    // ==================================================
    for (size_t i = 0; i < doors.size(); ++i) {
        glm::ivec2 centro = MundoACelda(doors[i]->GetPosition());
        for (int df = -1; df <= 1; ++df) {
            for (int dc = -1; dc <= 1; ++dc) {
                glm::ivec2 celda = centro + glm::ivec2(dc, df);
                if (celda.x >= 0 && celda.x < (int)laberinto[0].size() &&
                    celda.y >= 0 && celda.y < (int)laberinto.size()) {
                    if (laberinto[celda.y][celda.x] == 4) {
                        celdaAPuertaIdx[celda] = (int)i;
                    }
                }
            }
        }
    }


    // --- DIAGNÓSTICO DE PUERTAS ---
    std::cout << "\n=== DIAGNÓSTICO DE PUERTAS ===" << std::endl;
    std::cout << "Tamaño del mapa celdaAPuertaIdx: " << celdaAPuertaIdx.size() << std::endl;

    for (size_t i = 0; i < doors.size(); ++i) {
        glm::vec3 pos = doors[i]->GetPosition();
        glm::ivec2 celda = MundoACelda(pos);
        int valorEnMatriz = -1;
        if (celda.y >= 0 && celda.y < (int)laberinto.size() && celda.x >= 0 && celda.x < (int)laberinto[0].size())
            valorEnMatriz = laberinto[celda.y][celda.x];

        std::cout << "Puerta " << i << ": pos=(" << pos.x << ", " << pos.z << ") celda=(" << celda.x << "," << celda.y << ") valor=" << valorEnMatriz << std::endl;
    }
    std::cout << "==============================\n" << std::endl;

    // ==================================================
    // SWITCHES DE LUZ
    // ==================================================
    std::vector<std::unique_ptr<LightSwitch>> switches;
    switches.push_back(std::make_unique<LightSwitch>("Resources/Models/switch/switch_base.obj", "Resources/Models/switch/switch_rocker.obj", glm::vec3(-46.5f, 2.5f, -85.8093f), std::vector<int>{26, 35, 36}, 3.0f, 0.15f, true, 180.0f));
    switches.push_back(std::make_unique<LightSwitch>("Resources/Models/switch/switch_base.obj", "Resources/Models/switch/switch_rocker.obj", glm::vec3(-6.3f, 2.5f, -111.58f), std::vector<int>{26}, 3.0f, 0.15f, true, -90.0f));
    switches.push_back(std::make_unique<LightSwitch>("Resources/Models/switch/switch_base.obj", "Resources/Models/switch/switch_rocker.obj", glm::vec3(-56.35f, 2.5f, -5.0f), std::vector<int>{9, 10}, 3.0f, 0.15f, true, 0.0f));
    switches.push_back(std::make_unique<LightSwitch>("Resources/Models/switch/switch_base.obj", "Resources/Models/switch/switch_rocker.obj", glm::vec3(26.4432f, 2.5f, 4.1f), std::vector<int>{2, 3}, 3.0f, 0.15f, true, 90.0f, 180.0f));
    switches.push_back(std::make_unique<LightSwitch>("Resources/Models/switch/switch_base.obj", "Resources/Models/switch/switch_rocker.obj", glm::vec3(-29.0f, 2.5f, 23.57f), std::vector<int>{1}, 3.0f, 0.15f, true, 90.0f));

    std::map<glm::ivec2, int, IVec2Cmp> celdaASwitchIdx;
    for (int i = 0; i < (int)switches.size(); i++) {
        glm::ivec2 celda = MundoACelda(switches[i]->GetPosition());
        celdaASwitchIdx[celda] = i;
    }

    // ==================================================
    // ROPEROS
    // ==================================================
    glm::vec3 r_local_hingeIzq(0.895f, 0.0f, -0.873f);
    glm::vec3 r_local_hingeDer(-0.8992f, 0.0f, -0.509f);
    glm::vec3 r_local_interact(0.0f, 1.5f, -0.8f);

    struct PosicionRopero { glm::vec3 pos; float rot; };

    std::vector<PosicionRopero> ubicacionesRoperos = {
    { glm::vec3(-18.39f, -0.2f, -5.28f),    0.0f   },  // ropero original (correcto)
    { glm::vec3(13.95f,  -0.2f, -46.06f),   90.0f  },  // #2 → corregido
    { glm::vec3(37.71f,  -0.2f, 10.47f),    90.0f  },  // #3
    { glm::vec3(-32.05f, -0.2f, 13.98f),    270.0f },  // #4
    { glm::vec3(-11.98f, -0.2f, -109.99f),  180.0f },  // #1
    { glm::vec3(-74.08f, -0.2f, -86.45f),   270.0f },  // #5
    { glm::vec3(40.43f,  -0.2f, -71.16f),   90.0f  },  // #6
    { glm::vec3(44.65f,  -0.2f, -101.26f),  0.0f   }   // #7 → corregido
    };

    // --- INICIO BLOQUE CORREGIDO 1: Inicialización de roperos ---
    for (const auto& ubi : ubicacionesRoperos) {
        RoperoData rd;
        rd.worldPosition = ubi.pos;


        rd.modelMatrix = glm::rotate(glm::translate(glm::mat4(1.0f), ubi.pos), glm::radians(ubi.rot), glm::vec3(0, 1, 0));


        rd.openState = std::make_shared<bool>(false);

        glm::vec3 worldInteract = glm::vec3(rd.modelMatrix * glm::vec4(r_local_interact, 1.0f));
        glm::vec3 worldHingeIzq = r_local_hingeIzq;   // ← se mantienen en local
        glm::vec3 worldHingeDer = r_local_hingeDer;   // ← se mantienen en local

        rd.puertaIzq = std::make_unique<Door>(
            "Resources/Models/Casa/puerta_ropero_izquierda.obj",
            worldHingeIzq, worldInteract,
            0.0f, -120.0f, 150.0f, 2.0f, false, 2.5f,
            nullptr, rd.openState.get());

        // La puerta derecha no necesita interacción propia, por eso le pasamos una posición lejana
        rd.puertaDer = std::make_unique<Door>(
            "Resources/Models/Casa/puerta_ropero_derecha.obj",
            worldHingeDer, glm::vec3(0, -100, 0),
            0.0f, 120.0f, 150.0f, 2.0f, false, 2.5f,
            nullptr, rd.openState.get());

        roperosInstanciados.push_back(std::move(rd));
    }
    // --- FIN BLOQUE CORREGIDO 1 ---

    // ==================================================
    // LAMBDA: Planificar ruta a waypoint normal
    // ==================================================
    auto PlanificarNuevaRuta = [&]()
        {
            if (waypointsUnicos.empty() || laberinto.empty()) return;
            int filas = (int)laberinto.size();
            int columnas = (int)laberinto[0].size();

            int nuevoIdx = waypointDestinoIdx;
            if (waypointsUnicos.size() > 1) {
                while (nuevoIdx == waypointDestinoIdx)
                    nuevoIdx = rand() % (int)waypointsUnicos.size();
            }
            else nuevoIdx = 0;
            waypointDestinoIdx = nuevoIdx;
            glm::ivec2 destino = waypointsUnicos[waypointDestinoIdx];

            glm::ivec2 celdaInicio = MundoACelda(monsterPos);
            celdaInicio.x = std::max(0, std::min(celdaInicio.x, columnas - 1));
            celdaInicio.y = std::max(0, std::min(celdaInicio.y, filas - 1));

            if (laberinto[celdaInicio.y][celdaInicio.x] == 1) {
                bool encontrado = false;
                for (int radio = 1; radio <= 5 && !encontrado; ++radio)
                    for (int df = -radio; df <= radio && !encontrado; ++df)
                        for (int dc = -radio; dc <= radio && !encontrado; ++dc) {
                            int nf = celdaInicio.y + df, nc = celdaInicio.x + dc;
                            if (nf >= 0 && nf < filas && nc >= 0 && nc < columnas && laberinto[nf][nc] != 1) {
                                celdaInicio = glm::ivec2(nc, nf);
                                encontrado = true;
                            }
                        }
            }

            auto cruda = iaMonstruo.PlanificarRuta(celdaInicio, destino, laberinto, iaMonstruo.temperatura);
            if (cruda.empty()) { waypointDestinoIdx = -1; return; }
            rutaMundo.clear();
            for (auto& c : cruda) rutaMundo.push_back(CeldaAMundo(c, MONSTER_HEIGHT));
            indiceRuta = 0;
            monsterSwitchDestino = glm::ivec2(-1, -1);
            std::cout << "[IA] Nueva ruta -> waypoint " << waypointDestinoIdx << std::endl;
        };

    // ==================================================
    // LAMBDA: Decidir próximo destino en PATROL
    // ==================================================
    auto DecidirProximoDestino = [&]()
        {
            std::vector<int> switchesApagados;
            for (int i = 0; i < (int)switches.size(); i++)
                if (!switches[i]->isOn)
                    switchesApagados.push_back(i);

            bool irASwitch = !switchesApagados.empty() &&
                ((float)rand() / RAND_MAX) < PROB_IR_A_SWITCH;

            if (irASwitch) {
                int sIdx = switchesApagados[rand() % switchesApagados.size()];
                glm::ivec2 destino = MundoACelda(switches[sIdx]->GetPosition());

                glm::ivec2 celdaInicio = MundoACelda(monsterPos);
                int filas = (int)laberinto.size();
                int columnas = (int)laberinto[0].size();
                celdaInicio.x = std::max(0, std::min(celdaInicio.x, columnas - 1));
                celdaInicio.y = std::max(0, std::min(celdaInicio.y, filas - 1));

                auto cruda = iaMonstruo.PlanificarRuta(celdaInicio, destino, laberinto, iaMonstruo.temperatura);
                if (!cruda.empty()) {
                    rutaMundo.clear();
                    for (auto& c : cruda) rutaMundo.push_back(CeldaAMundo(c, MONSTER_HEIGHT));
                    indiceRuta = 0;
                    monsterSwitchDestino = destino;
                    std::cout << "[IA] Monstruo va a encender switch " << sIdx << std::endl;
                    return;
                }
            }

            PlanificarNuevaRuta();
        };

    PlanificarNuevaRuta();

    // ==================================================
    // ITEMS Y LLAVES
    // ==================================================
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

    // ==================================================
    // HUD
    // ==================================================
    HUD hud;
    if (!hud.Init("Resources/Fonts/arial.ttf", currentWindowWidth, currentWindowHeight))
        std::cout << "[ADVERTENCIA] HUD sin fuente." << std::endl;

    float keyPickedTimer = 0.0f, key2PickedTimer = 0.0f, key3PickedTimer = 0.0f;

    // ==================================================
    // SHADOW MAPS
    // ==================================================
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

    // ==================================================
    // CUBOS PARA LÁMPARAS Y MARKERS
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

    // --- VAO/VBO para el debug de visión ---
    glGenVertexArrays(1, &debugLineVAO);
    glGenBuffers(1, &debugLineVBO);

    // ==================================================
    // GAME LOOP
    // ==================================================
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (deltaTime > 0.05f) deltaTime = 0.05f;

        // Alternar músicas de fondo
        if (musicaFondoActual == 1 && !audio.IsPlaying("musica_fondo_1")) { audio.Play("musica_fondo_2"); musicaFondoActual = 2; }
        else if (musicaFondoActual == 2 && !audio.IsPlaying("musica_fondo_2")) { audio.Play("musica_fondo_1"); musicaFondoActual = 1; }

        // Temporizador música de escape
        if (escapeTimer > 0.0f) {
            escapeTimer -= deltaTime;
            if (escapeTimer <= 0.0f && audio.IsPlaying("musica_escape")) {
                audio.Stop("musica_escape");
                if (rand() % 2 == 0) { audio.Play("musica_fondo_1"); musicaFondoActual = 1; }
                else { audio.Play("musica_fondo_2"); musicaFondoActual = 2; }
            }
        }

        CheckFullscreenKey(window);

        // --- Capturar tecla T para ropero ---
        bool tPressed = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
        bool tPressedOnce = tPressed && !tPressedLastFrame;

        // --- LÓGICA DE OCULTACIÓN (salir con T) ---
        // --- INICIO BLOQUE CORREGIDO 2: Salir del ropero ---
        if (player.isHiding) {
            if (tPressedOnce && currentHideRopero) {
                std::cout << "[DEBUG] Saliendo del ropero..." << std::endl;
                player.ExitCloset();
                *currentHideRopero->openState = true;

                // Punto de salida en espacio local: delante del ropero
                glm::vec3 localExitPoint = glm::vec3(0.0f, 1.5f, -1.5f);
                glm::vec3 worldExitPoint = glm::vec3(currentHideRopero->modelMatrix * glm::vec4(localExitPoint, 1.0f));
                player.camera.Position = worldExitPoint;
                player.camera.Yaw = playerHideYaw;
                player.camera.Pitch = playerHidePitch;
                player.camera.updateCameraVectors();

                currentHideRopero = nullptr;
                tPressedOnce = false; // consumir
            }
        }
        // --- FIN BLOQUE CORREGIDO 2 ---

        else {
            // --- Jugador (si no está escondido) ---
            glm::vec3 oldPos = player.camera.Position;
            if (!player.isBlocked) player.ProcessInput(window, deltaTime);
            player.UpdatePhysics(deltaTime);
            player.UpdateFlashlight();

            // *** Listener comentado ***
            //audio.SetListenerPosition(player.camera.Position, player.camera.Front, player.camera.Up);

            if (player.softBreathEvent) audio.Play("medio_cansado");
            if (player.hardBreathEvent) audio.Play("cansado_completo");

            // Colisiones jugador (matriz)
            if (!laberinto.empty() && player.camera.Position.y < (NIVEL_DEL_SUELO + 3.0f))
            {
                float mg = 0.15f, bz = -player.camera.Position.z;
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

            // Colisión del jugador con puertas cerradas (basada en la matriz)
            if (ColisionConPuertas(player.camera.Position, 0.15f, doors, celdaAPuertaIdx)) {
                player.camera.Position.x = oldPos.x;
                player.camera.Position.z = oldPos.z;
            }
        }

        // ==================================================
        // DETECCIÓN DEL JUGADOR
        // ==================================================
        float luzJugador = CalcularLuzJugador(player.camera.Position, lightSystem, player.flashlight.on);
        float distAlJugador = glm::length(glm::vec2(monsterPos.x - player.camera.Position.x, monsterPos.z - player.camera.Position.z));
        bool deteccionProximidad = (distAlJugador <= ATTACK_RANGE);
        bool deteccionCercana = (distAlJugador <= AWARENESS_RADIUS);
        bool jugadorDetectado = deteccionProximidad || deteccionCercana || MonstruoDetectaJugador(
            monsterPos, monsterDirActual, player.camera.Position,
            luzJugador, laberinto,
            MONSTER_VISION_RANGE, MONSTER_VISION_ANGLE,
            MONSTER_VISION_RAYS, MONSTER_LIGHT_THRESHOLD);

        // El jugador escondido en un ropero es indetectable: anula
        // cualquier detección por proximidad, awareness o cono de visión.
        if (player.isHiding) jugadorDetectado = false;

        {
            static float debugVisionTimer = 0.0f;
            debugVisionTimer += deltaTime;
            if (debugVisionTimer >= 0.5f) {
                debugVisionTimer = 0.0f;
                std::cout << "[VISION] Luz: " << luzJugador << " | Detectado: " << (jugadorDetectado ? "SI" : "no") << " | Estado: " << (int)monsterState << std::endl;
            }
        }

        // ==================================================
        // MÁQUINA DE ESTADOS DEL MONSTRUO
        // ==================================================
        oldMonsterPos = monsterPos;

        switch (monsterState)
        {
        case MonsterState::PATROL:
        {
            if (jugadorDetectado) {
                detectionTimer += deltaTime;
                if (detectionTimer >= DETECTION_TIME_REQUIRED) {
                    monsterState = MonsterState::PURSUIT;
                    pursuitTimer = 0.0f;
                    detectionTimer = 0.0f;
                    lastKnownPlayerPos = player.camera.Position;
                    iaMonstruo.temperatura = 0.0f;

                    if (audio.IsPlaying("musica_fondo_1")) audio.Stop("musica_fondo_1");
                    if (audio.IsPlaying("musica_fondo_2")) audio.Stop("musica_fondo_2");
                    if (!audio.IsPlaying("musica_escape")) audio.Play("musica_escape");

                    // Usamos risa_fondo_extra en lugar de monstruo_risa
                    float volMonstruo = glm::clamp(1.0f - (distAlJugador / 50.0f), 0.0f, 1.0f);
                    if (!audio.IsPlaying("risa_fondo_extra")) {
                        audio.SetVolume("risa_fondo_extra", volMonstruo * 0.6f);
                        audio.Play("risa_fondo_extra");
                    }
                    audio.SetVolume("monstruo_alerta", volMonstruo * 0.7f);
                    audio.Play("monstruo_alerta");

                    glm::ivec2 celdaJugador = MundoACelda(player.camera.Position);
                    glm::ivec2 celdaInicio = MundoACelda(monsterPos);
                    celdaInicio.x = std::max(0, std::min(celdaInicio.x, (int)laberinto[0].size() - 1));
                    celdaInicio.y = std::max(0, std::min(celdaInicio.y, (int)laberinto.size() - 1));
                    celdaJugador.x = std::max(0, std::min(celdaJugador.x, (int)laberinto[0].size() - 1));
                    celdaJugador.y = std::max(0, std::min(celdaJugador.y, (int)laberinto.size() - 1));
                    auto cruda = iaMonstruo.PlanificarRuta(celdaInicio, celdaJugador, laberinto, 0.0f);
                    rutaMundo.clear();
                    for (auto& c : cruda) rutaMundo.push_back(CeldaAMundo(c, MONSTER_HEIGHT));
                    indiceRuta = 0;
                    monsterSwitchDestino = glm::ivec2(-1, -1);
                    monsterModel.SetAnimation("run");
                    std::cout << "[IA] PERSECUCION iniciada!" << std::endl;
                }
            }
            else {
                detectionTimer = 0.0f;
            }

            // --- Forcejeo ---
            if (forcejeando)
            {
                forcejeoTimer += deltaTime;
                monsterModel.SetAnimation("idle");
                if (!sonidoForcejeoReproducido) {
                    float distF = glm::length(player.camera.Position - monsterPos);
                    float volF = glm::clamp(1.0f - (distF / 250.0f), 0.0f, 1.0f);
                    audio.SetVolume("forcejeo", volF * 6.7f);
                    audio.Play("forcejeo");
                    sonidoForcejeoReproducido = true;
                    std::cout << "[IA] Monstruo forcejeando puerta..." << std::endl;
                }

                if (forcejeoTimer >= 2.0f) {
                    if (puertaForcejeando) {
                        bool abiertaAntes = puertaForcejeando->IsOpen();
                        puertaForcejeando->ForceOpen();
                        if (puertaForcejeando->IsOpen() && !abiertaAntes) {
                            std::cout << "[IA] Monstruo abrió la puerta." << std::endl;
                        }
                        else {
                            std::cout << "[IA] Monstruo no pudo abrir la puerta (bloqueada). Desiste." << std::endl;
                            rutaMundo.clear(); indiceRuta = 0;
                            DecidirProximoDestino();
                        }
                    }
                    forcejeando = false;
                    puertaForcejeando = nullptr;
                    sonidoForcejeoReproducido = false;
                    forcejeoTimer = 0.0f;
                    audio.Stop("forcejeo");
                }
                break;
            }

            {
                glm::vec3 dirDeseada = iaMonstruo.CalcularDireccionSteering(monsterPos, rutaMundo, indiceRuta, MONSTER_RADIO_ACEPTACION);
                bool rutaTerminada = (indiceRuta >= rutaMundo.size());

                if (!rutaTerminada && glm::length(dirDeseada) > 0.001f) {
                    monsterDirActual = glm::normalize(glm::mix(monsterDirActual, dirDeseada, MONSTER_DIR_LERP));
                    glm::vec3 nuevaPos = monsterPos + monsterDirActual * MONSTER_SPEED * deltaTime;
                    nuevaPos.y = MONSTER_HEIGHT;

                    // Detección de puerta cercana (solo para iniciar forcejeo)
                    bool cercaDePuerta = false;
                    for (auto& d : doors) {
                        if (d->IsOpen()) continue;
                        float dist = glm::length(glm::vec2(nuevaPos.x - d->GetPosition().x, nuevaPos.z - d->GetPosition().z));
                        if (dist < 1.5f) {
                            glm::vec3 haciaPuerta = glm::normalize(glm::vec3(d->GetPosition().x, nuevaPos.y, d->GetPosition().z) - monsterPos);
                            if (glm::dot(monsterDirActual, haciaPuerta) > 0.5f) {
                                forcejeando = true;
                                puertaForcejeando = d.get();
                                sonidoForcejeoReproducido = false;
                                forcejeoTimer = 0.0f;
                                std::cout << "[IA] Monstruo encontró puerta cerrada, iniciando forcejeo..." << std::endl;
                                cercaDePuerta = true;
                                break;
                            }
                        }
                    }
                    if (cercaDePuerta) break;

                    // Colisión con matriz
                    bool colision = false;
                    {
                        float margen = 0.05f, bz = -nuevaPos.z;
                        int fMin = (int)floor(((bz - margen) - OFFSET_Z) / TAMANO_BLOQUE);
                        int fMax = (int)floor(((bz + margen) - OFFSET_Z) / TAMANO_BLOQUE);
                        int cIzq = (int)floor(((nuevaPos.x - margen) - OFFSET_X) / TAMANO_BLOQUE);
                        int cDer = (int)floor(((nuevaPos.x + margen) - OFFSET_X) / TAMANO_BLOQUE);
                        int filas = (int)laberinto.size(), cols = (int)laberinto[0].size();
                        if (fMin < 0 || fMax >= filas || cIzq < 0 || cDer >= cols) colision = true;
                        else if (laberinto[fMin][cIzq] == 1 || laberinto[fMin][cDer] == 1 ||
                            laberinto[fMax][cIzq] == 1 || laberinto[fMax][cDer] == 1) colision = true;
                    }
                    // Colisión con puertas cerradas (matriz)
                    if (!colision && ColisionConPuertas(nuevaPos, 0.05f, doors, celdaAPuertaIdx))
                        colision = true;

                    if (!colision) monsterPos = nuevaPos;
                    else { rutaMundo.clear(); indiceRuta = 0; DecidirProximoDestino(); }
                }
                else if (rutaTerminada) {
                    if (monsterSwitchDestino.x != -1) {
                        auto it = celdaASwitchIdx.find(monsterSwitchDestino);
                        if (it != celdaASwitchIdx.end()) {
                            int sIdx = it->second;
                            if (!switches[sIdx]->isOn) {
                                switches[sIdx]->SetOn(true, lightSystem.lampEnabled);
                                std::cout << "[IA] Monstruo encendio switch " << sIdx << std::endl;
                            }
                        }
                        monsterSwitchDestino = glm::ivec2(-1, -1);
                    }
                    iaMonstruo.temperatura = 0.3f;
                    DecidirProximoDestino();
                }

                float dist = glm::length(glm::vec2(monsterPos.x - oldMonsterPos.x, monsterPos.z - oldMonsterPos.z));
                monsterModel.SetAnimation(dist > 0.001f ? "walk" : "idle");
            }
            break;
        }

        case MonsterState::PURSUIT:
        {
            if (jugadorDetectado) {
                lastKnownPlayerPos = player.camera.Position;
                pursuitTimer = 0.0f;
                static float repathTimer = 0.0f;
                repathTimer += deltaTime;
                if (repathTimer >= 0.2f) {
                    repathTimer = 0.0f;
                    glm::ivec2 celdaJugador = MundoACelda(player.camera.Position);
                    glm::ivec2 celdaInicio = MundoACelda(monsterPos);
                    celdaInicio.x = std::max(0, std::min(celdaInicio.x, (int)laberinto[0].size() - 1));
                    celdaInicio.y = std::max(0, std::min(celdaInicio.y, (int)laberinto.size() - 1));
                    celdaJugador.x = std::max(0, std::min(celdaJugador.x, (int)laberinto[0].size() - 1));
                    celdaJugador.y = std::max(0, std::min(celdaJugador.y, (int)laberinto.size() - 1));
                    auto cruda = iaMonstruo.PlanificarRuta(celdaInicio, celdaJugador, laberinto, 0.0f);
                    rutaMundo.clear();
                    for (auto& c : cruda) rutaMundo.push_back(CeldaAMundo(c, MONSTER_HEIGHT));
                    indiceRuta = 0;
                }
            }
            else {
                pursuitTimer += deltaTime;
                if (pursuitTimer >= PURSUIT_TIMEOUT) {
                    monsterState = MonsterState::PATROL;
                    pursuitTimer = 0.0f;
                    iaMonstruo.temperatura = 0.3f;
                    PlanificarNuevaRuta();
                    monsterModel.SetAnimation("walk");
                    escapeTimer = 10.0f;
                    std::cout << "[IA] Perdio al jugador. Volviendo a patrullaje." << std::endl;
                }
            }

            // --- Forcejeo ---
            if (forcejeando)
            {
                forcejeoTimer += deltaTime;
                monsterModel.SetAnimation("idle");
                if (!sonidoForcejeoReproducido) {
                    float distF = glm::length(player.camera.Position - monsterPos);
                    float volF = glm::clamp(1.0f - (distF / 250.0f), 0.0f, 1.0f);
                    audio.SetVolume("forcejeo", volF * 6.7f);
                    audio.Play("forcejeo");
                    sonidoForcejeoReproducido = true;
                    std::cout << "[IA] Monstruo forcejeando puerta..." << std::endl;
                }

                if (forcejeoTimer >= 2.0f) {
                    if (puertaForcejeando) {
                        bool abiertaAntes = puertaForcejeando->IsOpen();
                        puertaForcejeando->ForceOpen();
                        if (puertaForcejeando->IsOpen() && !abiertaAntes) {
                            std::cout << "[IA] Monstruo abrió la puerta." << std::endl;
                        }
                        else {
                            std::cout << "[IA] Monstruo no pudo abrir la puerta (bloqueada). Desiste." << std::endl;
                            rutaMundo.clear(); indiceRuta = 0;
                            DecidirProximoDestino();
                        }
                    }
                    forcejeando = false;
                    puertaForcejeando = nullptr;
                    sonidoForcejeoReproducido = false;
                    forcejeoTimer = 0.0f;
                    audio.Stop("forcejeo");
                }
                break;
            }

            if (distAlJugador <= ATTACK_RANGE) {
                attackRangeTimer += deltaTime;
                if (attackRangeTimer >= ATTACK_RANGE_TIME) {
                    monsterState = MonsterState::ATTACK;
                    attackRangeTimer = 0.0f;
                    attackIdleTimer = 0.0f;
                    attackAnimDone = false;
                    rutaMundo.clear();
                    monsterModel.SetAnimation("attack", false);
                    glm::vec3 dir = player.camera.Position - monsterPos;
                    dir.y = 0.0f;
                    if (glm::length(dir) > 0.001f) monsterDirActual = glm::normalize(dir);
                    glm::vec3 dirCam = monsterPos - player.camera.Position;
                    dirCam.y = 0.0f;
                    if (glm::length(dirCam) > 0.001f) {
                        dirCam = glm::normalize(dirCam);
                        player.camera.Yaw = glm::degrees(atan2(dirCam.x, dirCam.z)) - 90.0f;
                        player.camera.Pitch = 0.0f;
                        player.camera.updateCameraVectors();
                    }
                    player.isBlocked = true;
                    std::cout << "[IA] ATAQUE iniciado!" << std::endl;
                }
            }
            else {
                attackRangeTimer = 0.0f;
            }

            {
                glm::vec3 dirDeseada = iaMonstruo.CalcularDireccionSteering(monsterPos, rutaMundo, indiceRuta, MONSTER_RADIO_ACEPTACION);
                if (glm::length(dirDeseada) > 0.001f) {
                    monsterDirActual = glm::normalize(glm::mix(monsterDirActual, dirDeseada, MONSTER_DIR_LERP));
                    glm::vec3 nuevaPos = monsterPos + monsterDirActual * PURSUIT_SPEED * deltaTime;
                    nuevaPos.y = MONSTER_HEIGHT;

                    // Detección de puerta cercana
                    bool cercaDePuerta = false;
                    for (auto& d : doors) {
                        if (d->IsOpen()) continue;
                        float dist = glm::length(glm::vec2(nuevaPos.x - d->GetPosition().x, nuevaPos.z - d->GetPosition().z));
                        if (dist < 1.5f) {
                            glm::vec3 haciaPuerta = glm::normalize(glm::vec3(d->GetPosition().x, nuevaPos.y, d->GetPosition().z) - monsterPos);
                            if (glm::dot(monsterDirActual, haciaPuerta) > 0.5f) {
                                forcejeando = true;
                                puertaForcejeando = d.get();
                                sonidoForcejeoReproducido = false;
                                forcejeoTimer = 0.0f;
                                std::cout << "[IA] Monstruo encontró puerta cerrada, iniciando forcejeo..." << std::endl;
                                cercaDePuerta = true;
                                break;
                            }
                        }
                    }
                    if (cercaDePuerta) break;

                    // Colisión con matriz
                    bool colision = false;
                    {
                        float margen = 0.05f, bz = -nuevaPos.z;
                        int fMin = (int)floor(((bz - margen) - OFFSET_Z) / TAMANO_BLOQUE);
                        int fMax = (int)floor(((bz + margen) - OFFSET_Z) / TAMANO_BLOQUE);
                        int cIzq = (int)floor(((nuevaPos.x - margen) - OFFSET_X) / TAMANO_BLOQUE);
                        int cDer = (int)floor(((nuevaPos.x + margen) - OFFSET_X) / TAMANO_BLOQUE);
                        int filas = (int)laberinto.size(), cols = (int)laberinto[0].size();
                        if (fMin < 0 || fMax >= filas || cIzq < 0 || cDer >= cols) colision = true;
                        else if (laberinto[fMin][cIzq] == 1 || laberinto[fMin][cDer] == 1 ||
                            laberinto[fMax][cIzq] == 1 || laberinto[fMax][cDer] == 1) colision = true;
                    }
                    // Colisión con puertas cerradas (matriz)
                    if (!colision && ColisionConPuertas(nuevaPos, 0.05f, doors, celdaAPuertaIdx))
                        colision = true;

                    if (!colision) monsterPos = nuevaPos;
                }
                monsterModel.SetAnimation("run");
            }
            break;
        }

        case MonsterState::ATTACK:
        {
            if (!attackAnimDone) {
                const Animation& anim = monsterModel.GetAnimations()[monsterModel.currentAnim];
                double ticksPerSec = anim.ticksPerSecond > 0.0 ? anim.ticksPerSecond : 25.0;
                double totalTime = anim.duration / ticksPerSec;
                if (monsterModel.animTime >= (float)totalTime - 0.05f) {
                    attackAnimDone = true;

                    // ========== SISTEMA DE VIDAS ==========
                    attackIdleTimer = 0.0f;
                    monsterModel.SetAnimation("idle");
                    std::cout << "[IA] Ataque completado. Idle 2 segundos." << std::endl;

                    // Quitar vida y resetear
                    playerLives--;
                    std::cout << "[VIDAS] Vidas restantes: " << playerLives << std::endl;

                    if (playerLives <= 0) {
                        // Reiniciar el juego (cerrar ventana)
                        glfwSetWindowShouldClose(window, true);
                    }
                    else {
                        // Resetear posición del jugador
                        player.camera.Position = glm::vec3(-56.0f, 2.0f, -86.807f);
                        player.camera.Yaw = -90.0f;
                        player.camera.Pitch = 0.0f;
                        player.camera.updateCameraVectors();
                        player.isBlocked = false;
                        player.isHiding = false;

                        // Resetear monstruo
                        monsterPos = glm::vec3(129.0f, MONSTER_HEIGHT, -98.0f);
                        monsterDirActual = glm::vec3(1.0f, 0.0f, 0.0f);
                        monsterState = MonsterState::PATROL;
                        detectionTimer = 0.0f;
                        pursuitTimer = 0.0f;
                        attackRangeTimer = 0.0f;
                        attackIdleTimer = 0.0f;
                        cooldownTimer = 0.0f;
                        attackAnimDone = false;
                        forcejeando = false;
                        puertaForcejeando = nullptr;
                        sonidoForcejeoReproducido = false;
                        forcejeoTimer = 0.0f;
                        rutaMundo.clear();
                        indiceRuta = 0;
                        monsterSwitchDestino = glm::ivec2(-1, -1);
                        iaMonstruo.temperatura = 0.3f;
                        monsterModel.SetAnimation("idle");
                        PlanificarNuevaRuta();

                        // Resetear audio de escape
                        if (audio.IsPlaying("musica_escape")) audio.Stop("musica_escape");
                        if (!audio.IsPlaying("musica_fondo_1") && !audio.IsPlaying("musica_fondo_2")) {
                            if (rand() % 2 == 0) { audio.Play("musica_fondo_1"); musicaFondoActual = 1; }
                            else { audio.Play("musica_fondo_2"); musicaFondoActual = 2; }
                        }
                        escapeTimer = 0.0f;
                    }
                    // =====================================
                }
            }
            else {
                attackIdleTimer += deltaTime;
                if (attackIdleTimer >= ATTACK_IDLE_TIME) {
                    monsterState = MonsterState::ATTACK_COOLDOWN;
                    cooldownTimer = 0.0f;
                    monsterModel.SetAnimation("walk");
                    iaMonstruo.temperatura = 0.3f;
                    PlanificarNuevaRuta();
                    player.isBlocked = false;
                    escapeTimer = 10.0f;
                    std::cout << "[IA] Cooldown de " << ATTACK_COOLDOWN_TIME << "s iniciado." << std::endl;
                }
            }
            break;
        }

        case MonsterState::ATTACK_COOLDOWN:
        {
            // --- Forcejeo ---
            if (forcejeando)
            {
                forcejeoTimer += deltaTime;
                monsterModel.SetAnimation("idle");
                if (!sonidoForcejeoReproducido) {
                    float distF = glm::length(player.camera.Position - monsterPos);
                    float volF = glm::clamp(1.0f - (distF / 250.0f), 0.0f, 1.0f);
                    audio.SetVolume("forcejeo", volF * 6.7f);   // <-- igualado a los otros estados
                    audio.Play("forcejeo");
                    sonidoForcejeoReproducido = true;
                    std::cout << "[IA] Monstruo forcejeando puerta..." << std::endl;
                }

                if (forcejeoTimer >= 2.0f) {
                    if (puertaForcejeando) {
                        bool abiertaAntes = puertaForcejeando->IsOpen();
                        puertaForcejeando->ForceOpen();
                        if (puertaForcejeando->IsOpen() && !abiertaAntes) {
                            std::cout << "[IA] Monstruo abrió la puerta." << std::endl;
                        }
                        else {
                            std::cout << "[IA] Monstruo no pudo abrir la puerta (bloqueada). Desiste." << std::endl;
                            rutaMundo.clear(); indiceRuta = 0;
                            PlanificarNuevaRuta();
                        }
                    }
                    forcejeando = false;
                    puertaForcejeando = nullptr;
                    sonidoForcejeoReproducido = false;
                    forcejeoTimer = 0.0f;
                    audio.Stop("forcejeo");
                }
                break;
            }

            cooldownTimer += deltaTime;
            glm::vec3 dirDeseada = iaMonstruo.CalcularDireccionSteering(monsterPos, rutaMundo, indiceRuta, MONSTER_RADIO_ACEPTACION);
            bool rutaTerminada = (indiceRuta >= rutaMundo.size());
            if (!rutaTerminada && glm::length(dirDeseada) > 0.001f) {
                monsterDirActual = glm::normalize(glm::mix(monsterDirActual, dirDeseada, MONSTER_DIR_LERP));
                glm::vec3 nuevaPos = monsterPos + monsterDirActual * MONSTER_SPEED * deltaTime;
                nuevaPos.y = MONSTER_HEIGHT;

                // Detección de puerta cercana
                bool cercaDePuerta = false;
                for (auto& d : doors) {
                    if (d->IsOpen()) continue;
                    float dist = glm::length(glm::vec2(nuevaPos.x - d->GetPosition().x, nuevaPos.z - d->GetPosition().z));
                    if (dist < 1.5f) {
                        glm::vec3 haciaPuerta = glm::normalize(glm::vec3(d->GetPosition().x, nuevaPos.y, d->GetPosition().z) - monsterPos);
                        if (glm::dot(monsterDirActual, haciaPuerta) > 0.5f) {
                            forcejeando = true;
                            puertaForcejeando = d.get();
                            sonidoForcejeoReproducido = false;
                            forcejeoTimer = 0.0f;
                            std::cout << "[IA] Monstruo encontró puerta cerrada, iniciando forcejeo..." << std::endl;
                            cercaDePuerta = true;
                            break;
                        }
                    }
                }
                if (cercaDePuerta) break;

                // Colisión con matriz
                bool colision = false;
                {
                    float margen = 0.05f, bz = -nuevaPos.z;
                    int fMin = (int)floor(((bz - margen) - OFFSET_Z) / TAMANO_BLOQUE);
                    int fMax = (int)floor(((bz + margen) - OFFSET_Z) / TAMANO_BLOQUE);
                    int cIzq = (int)floor(((nuevaPos.x - margen) - OFFSET_X) / TAMANO_BLOQUE);
                    int cDer = (int)floor(((nuevaPos.x + margen) - OFFSET_X) / TAMANO_BLOQUE);
                    int filas = (int)laberinto.size(), cols = (int)laberinto[0].size();
                    if (fMin < 0 || fMax >= filas || cIzq < 0 || cDer >= cols) colision = true;
                    else if (laberinto[fMin][cIzq] == 1 || laberinto[fMin][cDer] == 1 ||
                        laberinto[fMax][cIzq] == 1 || laberinto[fMax][cDer] == 1) colision = true;
                }
                // Colisión con puertas cerradas (matriz)
                if (!colision && ColisionConPuertas(nuevaPos, 0.05f, doors, celdaAPuertaIdx))
                    colision = true;

                if (!colision) monsterPos = nuevaPos;
                else { rutaMundo.clear(); indiceRuta = 0; PlanificarNuevaRuta(); }
            }
            else if (rutaTerminada) {
                PlanificarNuevaRuta();
            }
            float dist = glm::length(glm::vec2(monsterPos.x - oldMonsterPos.x, monsterPos.z - oldMonsterPos.z));
            monsterModel.SetAnimation(dist > 0.001f ? "walk" : "idle");
            if (cooldownTimer >= ATTACK_COOLDOWN_TIME) {
                monsterState = MonsterState::PATROL;
                std::cout << "[IA] Cooldown terminado. Volviendo a patrullaje." << std::endl;
            }
            break;
        }
        } // fin switch

        monsterModel.Update(deltaTime);

        // Sonido de pasos del monstruo (bucle continuo, como el jugador)
        // Sonido de pasos del monstruo (bucle continuo, como el jugador)
        {
            float monsterSpeed = glm::length(glm::vec2(monsterPos.x - oldMonsterPos.x, monsterPos.z - oldMonsterPos.z)) / deltaTime;
            bool monsterMoving = (monsterSpeed > 0.1f && monsterState != MonsterState::ATTACK && !forcejeando);

            // Umbral dinámico: punto medio entre velocidad de patrulla y persecución
            const float umbralCorrer = (MONSTER_SPEED + PURSUIT_SPEED) * 0.5f;

            if (monsterMoving) {
                float dist = glm::length(player.camera.Position - monsterPos);
                float maxDist = 50.0f;
                float vol = glm::clamp(1.0f - (dist / maxDist), 0.0f, 1.0f);

                if (monsterSpeed > umbralCorrer) {  // corriendo
                    if (audio.IsPlaying("pasos_monstruo")) {
                        audio.Stop("pasos_monstruo");
                    }
                    if (!audio.IsPlaying("correr_monstruo")) {
                        audio.SetVolume("correr_monstruo", vol * 1.0f);
                        audio.Play("correr_monstruo");
                    }
                    else {
                        audio.SetVolume("correr_monstruo", vol * 1.0f);
                    }
                }
                else {  // caminando
                    if (audio.IsPlaying("correr_monstruo")) {
                        audio.Stop("correr_monstruo");
                    }
                    if (!audio.IsPlaying("pasos_monstruo")) {
                        audio.SetVolume("pasos_monstruo", vol * 1.0f);
                        audio.Play("pasos_monstruo");
                    }
                    else {
                        audio.SetVolume("pasos_monstruo", vol * 1.0f);
                    }
                }
            }
            else {
                if (audio.IsPlaying("pasos_monstruo")) audio.Stop("pasos_monstruo");
                if (audio.IsPlaying("correr_monstruo")) audio.Stop("correr_monstruo");
            }
        }

        // Debug posición monstruo (cada 1s)
        {
            static float debugTimer = 0.0f;
            debugTimer += deltaTime;
            if (debugTimer >= 1.0f) {
                debugTimer = 0.0f;
                int cx = (int)floor((monsterPos.x - OFFSET_X) / TAMANO_BLOQUE);
                int cz = (int)floor((-monsterPos.z - OFFSET_Z) / TAMANO_BLOQUE);
                if (cz >= 0 && cz < (int)laberinto.size() && cx >= 0 && cx < (int)laberinto[0].size())
                    std::cout << "[MONSTRUO] (" << monsterPos.x << "," << monsterPos.z << ") [" << cz << "," << cx << "]=" << laberinto[cz][cx] << std::endl;
            }
        }

        // Debug luces (tecla P)
        bool pPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (pPressed && !pPressedLastFrame) {
            std::cout << "\n--- Luces cercanas ---\nJugador: " << player.camera.Position.x << "," << player.camera.Position.y << "," << player.camera.Position.z << std::endl;
            for (int i = 0; i < NUM_LAMPS; i++) {
                float d = glm::length(player.camera.Position - lightSystem.lampPositions[i]);
                if (d < 35.0f) std::cout << "Luz[" << i << "] d=" << d << std::endl;
            }
            std::cout << "----------------------\n";
        }
        pPressedLastFrame = pPressed;

        // Debug visión monstruo (tecla L)
        bool lPressed = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
        if (lPressed && !lPressedLastFrame) debugVisionActive = !debugVisionActive;
        lPressedLastFrame = lPressed;

        // Debug posición jugador (tecla O) - para ubicar escondites/roperos
        bool oPressed = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;
        if (oPressed && !oPressedLastFrame) {
            std::cout << "\n[POS JUGADOR] x=" << player.camera.Position.x
                << " y=" << player.camera.Position.y
                << " z=" << player.camera.Position.z
                << " | Yaw=" << player.camera.Yaw << "\n" << std::endl;
        }
        oPressedLastFrame = oPressed;



        // Audio de movimiento del jugador (solo si no está escondido)
        if (!player.isHiding) {
            bool moving = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
            bool shiftPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
            bool running = moving && shiftPressed && !player.isExhausted && player.stamina > 0.0f;

            if (running) { if (audio.IsPlaying("caminata")) audio.Stop("caminata"); if (!audio.IsPlaying("correr")) audio.Play("correr"); }
            else if (moving) { if (audio.IsPlaying("correr")) audio.Stop("correr"); if (!audio.IsPlaying("caminata")) audio.Play("caminata"); }
            else { if (audio.IsPlaying("caminata")) audio.Stop("caminata"); if (audio.IsPlaying("correr")) audio.Stop("correr"); }
        }
        else {
            if (audio.IsPlaying("caminata")) audio.Stop("caminata");
            if (audio.IsPlaying("correr")) audio.Stop("correr");
        }

        // ==================================================
        // INTERACCIÓN: ITEMS + PUERTAS + SWITCHES + PALANCAS + ROPEROS
        // ==================================================
        bool ePressed = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
        bool ePressedOnce = ePressed && !ePressedLastFrame;

        bool hadKeyBefore = itemSystem.hasKey;
        bool hadKey2Before = itemSystem.hasKey2;
        bool hadKey3Before = itemSystem.hasKey3;

        itemSystem.Update(deltaTime, player.camera.Position, player.camera.Front, ePressed, ePressedLastFrame, &audio);

        if (!hadKeyBefore && itemSystem.hasKey)  keyPickedTimer = 3.0f;
        if (!hadKey2Before && itemSystem.hasKey2) key2PickedTimer = 3.0f;
        if (!hadKey3Before && itemSystem.hasKey3) key3PickedTimer = 3.0f;
        if (keyPickedTimer > 0.0f) keyPickedTimer -= deltaTime;
        if (key2PickedTimer > 0.0f) key2PickedTimer -= deltaTime;
        if (key3PickedTimer > 0.0f) key3PickedTimer -= deltaTime;

        // Actualizar puzzle de palancas
        puzzleCompleted = leverPuzzle.IsComplete();

        // Puertas normales
        for (size_t i = 0; i < doors.size(); ++i) {
            bool keyForThisDoor = false;
            if (i == 0) keyForThisDoor = itemSystem.hasKey;
            else if (i == 1) keyForThisDoor = itemSystem.hasKey2;
            else if (i == 4) keyForThisDoor = itemSystem.hasKey3;
            doors[i]->Update(deltaTime, player.camera.Position, player.camera.Front, ePressed, ePressedLastFrame, &audio, keyForThisDoor);
        }

        // ==================================================
        // ACCIÓN DE ESCONDERSE EN ROPERO (con T, puerta YA abierta)
        // ==================================================
        RoperoData* lookedRopero = nullptr;
        for (auto& r : roperosInstanciados) {
            if (r.puertaIzq->isBeingLookedAt || r.puertaDer->isBeingLookedAt) {
                lookedRopero = &r;
                break;
            }
        }

        // --- INICIO BLOQUE CORREGIDO 3: Esconderse en ropero ---
        if (!player.isHiding && lookedRopero && *lookedRopero->openState && tPressedOnce) {
            float dist = glm::distance(
                glm::vec2(player.camera.Position.x, player.camera.Position.z),
                glm::vec2(lookedRopero->worldPosition.x, lookedRopero->worldPosition.z)
            );
            if (dist < 2.5f) {
                std::cout << "[DEBUG] Escondiéndose en ropero..." << std::endl;
                // Punto de escondite en espacio local: centro del ropero, ligeramente atrás
                glm::vec3 localHidePoint = glm::vec3(0.0f, 1.0f, 0.5f);
                glm::vec3 worldHidePoint = glm::vec3(lookedRopero->modelMatrix * glm::vec4(localHidePoint, 1.0f));
                player.HideInCloset(worldHidePoint);
                currentHideRopero = lookedRopero;
                *lookedRopero->openState = false;

                playerHideYaw = player.camera.Yaw;
                playerHidePitch = player.camera.Pitch;
            }
        }
        // --- FIN BLOQUE CORREGIDO 3 ---


        // Actualizar puertas de los roperos (con E)
        for (auto& r : roperosInstanciados) {
            r.puertaIzq->Update(deltaTime, player.camera.Position, player.camera.Front, ePressed, ePressedLastFrame, &audio, false);
            r.puertaDer->Update(deltaTime, player.camera.Position, player.camera.Front, ePressed, ePressedLastFrame, &audio, false);
        }

        leverPuzzle.Update(player.camera.Position, ePressedOnce, deltaTime, &audio);

        for (auto& sw : switches)
            sw->Update(deltaTime, player.camera.Position, player.camera.Front, ePressed, ePressedLastFrame, &audio, lightSystem.lampEnabled);
        ePressedLastFrame = ePressed;
        tPressedLastFrame = tPressed;

        // HUD state
        HUDState hudState{};
        hudState.hasKey = itemSystem.hasKey;
        hudState.hasKey2 = itemSystem.hasKey2;
        hudState.hasKey3 = itemSystem.hasKey3;
        hudState.showKeyPickedMsg = (keyPickedTimer > 0.0f); hudState.keyPickedMsgTimer = keyPickedTimer;
        hudState.showKey2PickedMsg = (key2PickedTimer > 0.0f); hudState.key2PickedMsgTimer = key2PickedTimer;
        hudState.showKey3PickedMsg = (key3PickedTimer > 0.0f); hudState.key3PickedMsgTimer = key3PickedTimer;
        hudState.stamina = player.stamina;
        hudState.staminaMax = STAMINA_MAX;
        hudState.isExhausted = player.isExhausted;
        hudState.playerLives = playerLives;   // <-- MOSTRAR VIDAS EN HUD

        for (auto& sw : switches) {
            if (sw->isBeingLookedAt && !hudState.lookingAtSwitch) {
                hudState.lookingAtSwitch = true;
                hudState.switchIsOn = sw->isOn;
            }
        }

        {
            glm::vec2 playerXZ(player.camera.Position.x, player.camera.Position.z);
            for (auto& item : itemSystem.items) {
                if (!item.visible) continue;
                glm::vec2 itemXZ(item.position.x, item.position.z);
                if (glm::length(playerXZ - itemXZ) < item.pickupRadius) {
                    hudState.lookingAtItem = true;
                    break;
                }
            }
        }

        for (size_t i = 0; i < doors.size(); ++i) {
            if (doors[i]->isBeingLookedAt) {
                if (i >= doors.size() - 2)
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

        // Ropero HUD
        lookedRopero = nullptr;
        for (auto& r : roperosInstanciados) {
            if (r.puertaIzq->isBeingLookedAt || r.puertaDer->isBeingLookedAt) {
                lookedRopero = &r;
                break;
            }
        }
        if (lookedRopero) {
            hudState.lookingAtRopero = true;
            hudState.roperoOpen = *lookedRopero->openState;
            if (*lookedRopero->openState && !player.isHiding) {
                float dist = glm::distance(
                    glm::vec2(player.camera.Position.x, player.camera.Position.z),
                    glm::vec2(lookedRopero->worldPosition.x, lookedRopero->worldPosition.z)
                );
                hudState.canHideInRopero = (dist < 2.5f);
            }
        }
        hudState.playerHiding = player.isHiding;

        // Eventos de tensión / ambiente
        if (currentFrame >= nextTensionSoundTime) { audio.Play("sonido_tension");   nextTensionSoundTime = currentFrame + RandomRange(30.0f, 70.0f); }
        if (currentFrame >= nextLaughSoundTime) { audio.Play("risa_tension");     nextLaughSoundTime = currentFrame + RandomRange(90.0f, 180.0f); }
        if (currentFrame >= nextDoorKnockTime) { audio.Play("toque_puerta");     nextDoorKnockTime = currentFrame + 40.0f; }
        if (currentFrame >= nextRisaExtraTime) {
            float distRisa = glm::length(player.camera.Position - monsterPos);
            float volRisa = glm::clamp(1.0f - (distRisa / 50.0f), 0.0f, 1.0f);
            audio.SetVolume("risa_fondo_extra", volRisa * 0.4f);
            audio.Play("risa_fondo_extra");
            nextRisaExtraTime = currentFrame + RandomRange(40.0f, 80.0f);
        }
        if (currentFrame >= nextCorridaTime) { audio.Play("corrida_fondo");    nextCorridaTime = currentFrame + RandomRange(60.0f, 120.0f); }

        glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

        lightSystem.Update(deltaTime, currentFrame, player.camera.Position);
        if (lightSystem.lightsFlickering) { if (!audio.IsPlaying("sonido_luces")) audio.Play("sonido_luces"); }
        else { if (audio.IsPlaying("sonido_luces"))  audio.Stop("sonido_luces"); }

        // Matrices de sombra
        glm::mat4 flashLightSpaceMatrix = glm::perspective(glm::radians(55.0f), 1.0f, 0.05f, 50.0f) * glm::lookAt(player.flashlight.position, player.flashlight.position + player.flashlight.direction, glm::vec3(0, 1, 0));
        glm::vec3 slp = lightSystem.lampPositions[26];
        glm::mat4 lampLightSpaceMatrix = glm::perspective(glm::radians(110.0f), 1.0f, 0.1f, 38.0f) * glm::lookAt(slp, slp + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1));

        // Depth pass linterna
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, flashDepthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        depthShader.use();
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(flashLightSpaceMatrix));
        glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
        model->Draw(depthShader.ID);
        for (auto& d : doors) d->Draw(depthShader.ID, modelMat);
        for (auto& r : roperosInstanciados) {
            glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(r.modelMatrix));
            roperoCuerpoModel->Draw(depthShader.ID);
            r.puertaIzq->Draw(depthShader.ID, r.modelMatrix);
            r.puertaDer->Draw(depthShader.ID, r.modelMatrix);
        }
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
        for (auto& r : roperosInstanciados) {
            glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(r.modelMatrix));
            roperoCuerpoModel->Draw(depthShader.ID);
            r.puertaIzq->Draw(depthShader.ID, r.modelMatrix);
            r.puertaDer->Draw(depthShader.ID, r.modelMatrix);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ==================================================
        // RENDER NORMAL
        // ==================================================
        glViewport(0, 0, currentWindowWidth, currentWindowHeight);
        glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = GetViewWithStaminaEffect(player);
        float currentFov = player.camera.Zoom;
        if (player.isExhausted) currentFov = 52.0f;
        else if (player.GetStaminaPercent() <= STAMINA_LOW_PERCENT) currentFov = 48.0f;
        glm::mat4 projection = glm::perspective(glm::radians(currentFov), GetAspectRatio(), 0.1f, 1000.0f);

        glm::mat4 projView = projection * view;
        Frustum frustum = ExtractFrustum(projView);

        shader.use();
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "flashLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(flashLightSpaceMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader.ID, "lampLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lampLightSpaceMatrix));
        glUniform1i(glGetUniformLocation(shader.ID, "lampShadowEnabled"), 1);
        glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, flashDepthMap); glUniform1i(glGetUniformLocation(shader.ID, "flashShadowMap"), 10);
        glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_2D, lampDepthMap);  glUniform1i(glGetUniformLocation(shader.ID, "lampShadowMap"), 11);
        glUniform1i(glGetUniformLocation(shader.ID, "flashlightOn"), player.flashlight.on);
        glUniform3f(glGetUniformLocation(shader.ID, "flashlightPos"), player.flashlight.position.x, player.flashlight.position.y, player.flashlight.position.z);
        glUniform3f(glGetUniformLocation(shader.ID, "flashlightDir"), player.flashlight.direction.x, player.flashlight.direction.y, player.flashlight.direction.z);

        for (int i = 0; i < NUM_LAMPS; i++)
        {
            std::string b = "lights[" + std::to_string(i) + "]";
            float lin = 0.045f, quad = 0.014f, pwr = lightSystem.intensities[i] * 0.75f, range = 13.0f;
            if (i == 2 || i == 3 || i == 4) { lin = 0.09f;   quad = 0.032f;  pwr = lightSystem.intensities[i] * 0.65f; range = 9.0f; }
            if (i == 29 || i == 34) { lin = 0.018f;  quad = 0.0035f; pwr = lightSystem.intensities[i] * 1.5f;  range = 24.0f; }
            if (i == 26) { lin = 0.026f;  quad = 0.006f;  pwr = lightSystem.intensities[i] * 1.25f; range = 28.0f; }
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

        // Casa (con frustum culling)
        glm::vec3 houseWorldMin = houseLocalMin + glm::vec3(0.0f, -1.0f, 0.0f);
        glm::vec3 houseWorldMax = houseLocalMax + glm::vec3(0.0f, -1.0f, 0.0f);
        if (IsAABBInFrustum(frustum, houseWorldMin, houseWorldMax)) {
            glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
            model->Draw(shader.ID);
        }

        // Puertas (con frustum culling)
        for (size_t i = 0; i < doors.size(); ++i) {
            if (IsAABBInFrustum(frustum, doorAABBMin[i], doorAABBMax[i])) {
                doors[i]->Draw(shader.ID, modelMat);
            }
        }

        // Roperos
        for (auto& r : roperosInstanciados) {
            if (IsAABBInFrustum(frustum, r.worldPosition - glm::vec3(2.0f), r.worldPosition + glm::vec3(2.0f))) {
                glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(r.modelMatrix));
                roperoCuerpoModel->Draw(shader.ID);
                r.puertaIzq->Draw(shader.ID, r.modelMatrix);
                r.puertaDer->Draw(shader.ID, r.modelMatrix);
            }
        }

        // Palancas (siempre se dibujan)
        leverPuzzle.Draw(shader, modelMat);

        // Switches de luz
        for (auto& sw : switches) sw->Draw(shader.ID, modelMat);

        // ==================== MONSTRUO (modelo animado + IA) ====================
        {
            monsterShader.use();

            glUniformMatrix4fv(glGetUniformLocation(monsterShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(monsterShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(glGetUniformLocation(monsterShader.ID, "flashLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(flashLightSpaceMatrix));
            glUniformMatrix4fv(glGetUniformLocation(monsterShader.ID, "lampLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lampLightSpaceMatrix));

            glUniform3f(glGetUniformLocation(monsterShader.ID, "viewPos"), player.camera.Position.x, player.camera.Position.y, player.camera.Position.z);
            glUniform1i(glGetUniformLocation(monsterShader.ID, "flashlightOn"), player.flashlight.on);
            glUniform3f(glGetUniformLocation(monsterShader.ID, "flashlightPos"), player.flashlight.position.x, player.flashlight.position.y, player.flashlight.position.z);
            glUniform3f(glGetUniformLocation(monsterShader.ID, "flashlightDir"), player.flashlight.direction.x, player.flashlight.direction.y, player.flashlight.direction.z);
            glUniform1i(glGetUniformLocation(monsterShader.ID, "lampShadowEnabled"), 1);
            glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_2D, flashDepthMap); glUniform1i(glGetUniformLocation(monsterShader.ID, "flashShadowMap"), 10);
            glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_2D, lampDepthMap);  glUniform1i(glGetUniformLocation(monsterShader.ID, "lampShadowMap"), 11);

            for (int i = 0; i < NUM_LAMPS; i++) {
                std::string b = "lights[" + std::to_string(i) + "]";
                float lin = 0.045f, quad = 0.014f, pwr = lightSystem.intensities[i] * 0.75f, range = 13.0f;
                if (i == 2 || i == 3 || i == 4) { lin = 0.09f;  quad = 0.032f;  pwr = lightSystem.intensities[i] * 0.65f; range = 9.0f; }
                if (i == 29 || i == 34) { lin = 0.018f; quad = 0.0035f; pwr = lightSystem.intensities[i] * 1.5f;  range = 24.0f; }
                if (i == 26) { lin = 0.026f; quad = 0.006f;  pwr = lightSystem.intensities[i] * 1.25f; range = 28.0f; }
                glUniform3f(glGetUniformLocation(monsterShader.ID, (b + ".position").c_str()), lightSystem.lampPositions[i].x, lightSystem.lampPositions[i].y, lightSystem.lampPositions[i].z);
                glUniform3f(glGetUniformLocation(monsterShader.ID, (b + ".color").c_str()), 1.0f, 0.95f, 0.8f);
                glUniform1f(glGetUniformLocation(monsterShader.ID, (b + ".intensity").c_str()), pwr);
                glUniform1f(glGetUniformLocation(monsterShader.ID, (b + ".constant").c_str()), 1.0f);
                glUniform1f(glGetUniformLocation(monsterShader.ID, (b + ".linear").c_str()), lin);
                glUniform1f(glGetUniformLocation(monsterShader.ID, (b + ".quadratic").c_str()), quad);
                glUniform1f(glGetUniformLocation(monsterShader.ID, (b + ".range").c_str()), range);
            }

            float anguloY = atan2(monsterDirActual.x, monsterDirActual.z);
            glm::mat4 monsterMat = glm::mat4(1.0f);
            monsterMat = glm::translate(monsterMat, monsterPos);
            monsterMat = glm::rotate(monsterMat, anguloY, glm::vec3(0.0f, 1.0f, 0.0f));
            monsterMat = glm::scale(monsterMat, glm::vec3(MONSTER_DRAW_SCALE));

            glm::vec3 monsterWorldMin = monsterPos + MONSTER_LOCAL_AABB_MIN * MONSTER_DRAW_SCALE;
            glm::vec3 monsterWorldMax = monsterPos + MONSTER_LOCAL_AABB_MAX * MONSTER_DRAW_SCALE;

            if (IsAABBInFrustum(frustum, monsterWorldMin, monsterWorldMax)) {
                glUniformMatrix4fv(glGetUniformLocation(monsterShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(monsterMat));
                glUniform1i(glGetUniformLocation(monsterShader.ID, "hasDiffuseTexture"), 1);
                glUniform3f(glGetUniformLocation(monsterShader.ID, "materialColor"), 1.0f, 1.0f, 1.0f);
                monsterModel.Draw(monsterShader.ID);
            }
        }

        // ==================== DEBUG VISIÓN MONSTRUO ====================
        if (debugVisionActive) {
            const int NUM_RAYOS_DEBUG = 40;
            const float RANGO_CORTO = 10.0f;
            const float RANGO_LARGO = 20.0f;
            const float ANGULO_CONO = glm::radians(MONSTER_VISION_ANGLE);

            std::vector<glm::vec3> linePoints;

            glm::vec2 dirFlat = glm::normalize(glm::vec2(monsterDirActual.x, monsterDirActual.z));

            for (int r = 0; r < NUM_RAYOS_DEBUG; r++) {
                float t = -1.0f + 2.0f * r / (NUM_RAYOS_DEBUG - 1);
                float angulo = t * ANGULO_CONO * 0.5f;
                float cosA = cos(angulo), sinA = sin(angulo);
                glm::vec2 rayDir = glm::vec2(
                    dirFlat.x * cosA - dirFlat.y * sinA,
                    dirFlat.x * sinA + dirFlat.y * cosA);

                glm::vec3 origen = glm::vec3(monsterPos.x, 1.5f, monsterPos.z);  // altura fija visible
                glm::vec3 dirRayo = glm::vec3(rayDir.x, 0.0f, rayDir.y);

                // Segmento blanco: 0 a 10m
                linePoints.push_back(origen);
                linePoints.push_back(origen + dirRayo * RANGO_CORTO);

                // Segmento amarillo: 10m a 20m
                linePoints.push_back(origen + dirRayo * RANGO_CORTO);
                linePoints.push_back(origen + dirRayo * RANGO_LARGO);
            }

            glBindVertexArray(debugLineVAO);
            glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
            glBufferData(GL_ARRAY_BUFFER, linePoints.size() * sizeof(glm::vec3), linePoints.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
            glEnableVertexAttribArray(0);

            debugLineShader.use();
            glUniformMatrix4fv(glGetUniformLocation(debugLineShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(debugLineShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

            int mitad = NUM_RAYOS_DEBUG * 2; // cada rayo tiene 2 puntos por segmento

            // Dibujar segmentos blancos (0 a 10m)
            glUniform3f(glGetUniformLocation(debugLineShader.ID, "lineColor"), 1.0f, 1.0f, 1.0f);
            for (int r = 0; r < NUM_RAYOS_DEBUG; r++) {
                glDrawArrays(GL_LINES, r * 4, 2);
            }

            // Dibujar segmentos amarillos (10 a 20m)
            glUniform3f(glGetUniformLocation(debugLineShader.ID, "lineColor"), 1.0f, 1.0f, 0.0f);
            for (int r = 0; r < NUM_RAYOS_DEBUG; r++) {
                glDrawArrays(GL_LINES, r * 4 + 2, 2);
            }

            glBindVertexArray(0);
        }

        // Lámparas (con frustum culling)
        std::vector<int> lamparasActivas = { 1, 2, 3, 6, 8, 9, 10, 11, 13, 16, 18, 21, 23, 26, 29, 34, 35, 36 };
        lampShader.use();
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(lampVAO);
        for (int i : lamparasActivas)
        {
            glm::vec3 lampMin = lightSystem.lampPositions[i] - glm::vec3(0.5f, 0.05f, 0.2f);
            glm::vec3 lampMax = lightSystem.lampPositions[i] + glm::vec3(0.5f, 0.05f, 0.2f);
            if (!IsAABBInFrustum(frustum, lampMin, lampMax)) continue;

            glm::mat4 lm = glm::translate(glm::mat4(1.0f), lightSystem.lampPositions[i]);
            glUniformMatrix4fv(glGetUniformLocation(lampShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(lm));
            float br = lightSystem.intensities[i];
            if (!lightSystem.lampEnabled[i])
                glUniform3f(glGetUniformLocation(lampShader.ID, "lampColor"), 0.05f, 0.05f, 0.05f);
            else
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

    // ==================================================
    // LIMPIEZA
    // ==================================================
    delete model;
    delete roperoCuerpoModel;
    glDeleteVertexArrays(1, &lampVAO);
    glDeleteBuffers(1, &lampVBO);
    glDeleteVertexArrays(1, &markerVAO);
    glDeleteBuffers(1, &markerVBO);
    glDeleteFramebuffers(1, &flashDepthMapFBO);
    glDeleteFramebuffers(1, &lampDepthMapFBO);
    glDeleteTextures(1, &flashDepthMap);
    glDeleteTextures(1, &lampDepthMap);
    glDeleteVertexArrays(1, &debugLineVAO);
    glDeleteBuffers(1, &debugLineVBO);
    audio.Shutdown();
    glfwTerminate();
    return 0;
}