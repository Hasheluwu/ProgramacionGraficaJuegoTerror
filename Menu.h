#pragma once
#include <glad/glad.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "stb_truetype.h"

// ============================================================
//  Estados del menú (ampliados)
// ============================================================
enum class MenuState {
    MAIN,
    SETTINGS,
    CREDITS,
    LOADING,
    PLAYING,
    PAUSED,         // Pausa en juego
    CONFIRM_QUIT,   // Diálogo de confirmación al salir
    CONFIRM_MAIN    // Confirmar volver al menú principal
};

// ============================================================
//  Partícula simple para efectos de loading / fondo
// ============================================================
struct Particle {
    float x, y;
    float vx, vy;
    float life;      // 0..1
    float maxLife;
    float size;
    float alpha;
};

// ============================================================
//  Resultado de Update — indica qué acción tomó el usuario
// ============================================================
enum class MenuAction {
    NONE,
    START_GAME,
    RESUME_GAME,
    RESTART_GAME,
    GO_TO_MAIN,
    QUIT
};

// ============================================================
//  Clase Menu
// ============================================================
class Menu {
public:
    // --- Estado visible externamente ---
    MenuState   state = MenuState::MAIN;
    float       blinkTimer = 0.0f;
    float       loadingProgress = 0.0f;

    // --- Configuración accesible ---
    float   mouseSensitivity = 0.08f;
    float   brightness = 1.0f;    // 0.5 .. 2.0
    float   masterVolume = 1.0f;    // 0.0 .. 1.0
    double  mouseX = 0, mouseY = 0;
    int     hoveredItem = -1;
    int   selectedItem = 0;

    // --------------------------------------------------------
    Menu(int screenWidth, int screenHeight);
    ~Menu();

    // Actualiza lógica; devuelve la acción que debe tomar Main
    MenuAction  Update(GLFWwindow* window, float deltaTime);

    // Renderiza el estado actual
    void        Render();

    // Progreso de carga (0 .. 1)
    void        SetLoadingProgress(float p);

    // Permite a Main.cpp llamar renderSettings directamente
    // (mantenemos compatibilidad con el código original)
    void        renderSettings(GLFWwindow* window);

    // Carga imagen RGBA desde archivo (PNG/JPG vía stb_image)
    bool        loadImageTexture(const char* path,
        unsigned int& texOut,
        int& wOut, int& hOut);

private:
    // ---- Dimensiones ----
    int scrW, scrH;

    // ---- Timers ----
    float hoverTimer = 0.0f;
    float transTimer = 0.0f;   // para fade entre estados
    float fadeAlpha = 0.0f;   // 0=transparente 1=opaco
    bool  fadingIn = true;
    bool  fadingOut = false;
    MenuState pendingState = MenuState::MAIN;
    MenuState previousState = MenuState::MAIN;  // rastrear estado anterior para volver correctamente
    MenuAction pendingAction = MenuAction::NONE;  // acción a retornar cuando fade termine

    // ---- Mouse / selección ----
    bool  prevEnter = false;
    float parallaxX = 0.0f;   // offset parallax fondo
    float parallaxY = 0.0f;

    // ---- Fuentes STB ----
    static const int FONT_ATLAS_W = 512;
    static const int FONT_ATLAS_H = 512;

    stbtt_bakedchar cdataLarge[96];  // 52px — títulos
    stbtt_bakedchar cdataSmall[96];  // 28px — cuerpo
    unsigned int fontTexLarge = 0;
    unsigned int fontTexSmall = 0;
    bool fontLoaded = false;

    // ---- Texturas ----
    unsigned int bgTexture = 0;   // fondo menú principal
    int          bgW = 0, bgH = 0;

    unsigned int logoTexture = 0;   // logo universidad
    int          logoW = 0, logoH = 0;

    // ---- Shaders y VAO ----
    unsigned int quadShader = 0;
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;

    // Shader con soporte de textura RGBA (para imágenes)
    unsigned int imgShader = 0;

    // ---- Partículas ----
    std::vector<Particle> particles;
    void spawnParticle();
    void updateParticles(float dt);
    void renderParticles();

    // ---- Settings — sliders ----
    int   settingsSelected = 0;    // 0=sens 1=bright 2=vol
    bool  prevLeft = false;
    bool  prevRight = false;
    bool  prevUp = false;
    bool  prevEnterKey = false;
    bool  prevDown = false;
    int   sliderDragging = -1;     // índice del slider siendo arrastrado (-1 = ninguno)

    // ---- Credits scroll ----
    float creditsScroll = 0.0f;

    // ---- Internos de carga ----
    bool loadFont(const char* path);
    void compileQuadShader();
    void compileImgShader();
    void setupQuadBuffers();

    // ---- Render por estado ----
    void renderMain();
    void renderCredits();
    void renderLoading();
    void renderPause();
    void renderConfirmDialog(const std::string& msg, int& sel);
    void renderFadeOverlay();

    // ---- Primitivas 2D ----
    // Rect sólido
    void drawRect(float x, float y, float w, float h,
        float r, float g, float b, float a);

    // Rect con degradado vertical
    void drawRectGradient(float x, float y, float w, float h,
        float r0, float g0, float b0, float a0,
        float r1, float g1, float b1, float a1);

    // Imagen RGBA (usa imgShader)
    void drawImage(unsigned int tex, float x, float y,
        float w, float h, float alpha = 1.0f);

    // Texto — isLarge elige entre cdataLarge / cdataSmall
    void drawText(const std::string& text,
        float x, float y, float scale,
        float r, float g, float b, float a,
        bool isLarge = true);

    // Medición de ancho de texto en píxeles
    float measureText(const std::string& text,
        float scale, bool isLarge = true);

    // Slider horizontal
    void drawSlider(float x, float y, float w, float h,
        float value,   // 0..1 normalizado
        bool selected);

    // Convierte píxeles → NDC
    glm::vec2 toNDC(float x, float y);

    // Inicia transición fade a nuevo estado
    void startFade(MenuState next, MenuAction action = MenuAction::NONE);
    void updateFade(float dt);
};
