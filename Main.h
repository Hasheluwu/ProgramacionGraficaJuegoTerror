#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include "stb_truetype.h"

enum class MenuState { MAIN, SETTINGS, CREDITS, LOADING, PLAYING, INTRO};

struct TextQuad {
    float x, y, w, h;
    float s0, t0, s1, t1;
};

class Menu {
public:
    float blinkTimer = 0.0f;
    float loadingProgress = 0.0f;
    MenuState state = MenuState::MAIN;

    // Configuracion
    float mouseSensitivity = 0.08f;
    double mouseX = 0, mouseY = 0;
    int hoveredItem = -1;

    Menu(int screenWidth, int screenHeight);
    ~Menu();

    // Devuelve true cuando hay que entrar al juego
    bool Update(GLFWwindow* window, float deltaTime);
    void Render();

    // Progreso de carga (0.0 a 1.0)
    void SetLoadingProgress(float progress);
    void renderSettings(GLFWwindow* window);

private:
    int scrW, scrH;
    float hoverTimer = 0.0f;

    // Seleccion en menu principal
    int selectedItem = 0; // 0=Play 1=Settings 2=Credits 3=Quit
    bool prevUp = false, prevDown = false, prevEnter = false;

    // Fuente
    stbtt_bakedchar cdata[96];
    unsigned int fontTexture = 0;
    bool fontLoaded = false;

    // Shaders y VAO para quads 2D
    unsigned int quadShader = 0;
    unsigned int quadVAO = 0, quadVBO = 0;

    // ---- Internos ----
    bool loadFont(const char* path);
    void compileQuadShader();
    void setupQuadBuffers();

    void renderMain(float dt);
    void renderCredits();
    void renderLoading();

    // Dibuja un quad de color solido (coordenadas en pixeles)
    void drawRect(float x, float y, float w, float h,
        float r, float g, float b, float a);

    // Dibuja texto con la fuente STB
    void drawText(const std::string& text, float x, float y,
        float scale, float r, float g, float b, float a);

    // Convierte pixeles a NDC
    glm::vec2 toNDC(float x, float y);
};
