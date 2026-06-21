#pragma once


#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "stb_truetype.h"
#include "AudioManager.h"

struct IntroSlide
{
    std::string text;
    float displayTime;  // cuanto tiempo se muestra
};

class Intro
{
public:
    bool finished = false;

    Intro(int screenWidth, int screenHeight);
    ~Intro();

    bool Init(const std::string& fontPath);
    void Update(float deltaTime, AudioManager* audio);
    void Render();

private:
    int scrW, scrH;

    // Cursor parpadeante
    float cursorBlinkTimer = 0.0f;
    bool  cursorVisible = true;

    // Efecto niebla al terminar slide
    float fogTimer = 0.0f;
    float fogAlpha = 0.0f;
    bool  fogActive = false;

    // Slides de texto
    std::vector<IntroSlide> slides;
    int   currentSlide = 0;
    float slideTimer = 0.0f;

    // Fade
    float fadeAlpha = 0.0f;   // alpha del texto
    float fadeInTime = 1.5f;
    float fadeOutTime = 1.0f;
    bool  fadingIn = true;
    bool  fadingOut = false;

    // Efecto maquina de escribir
    float typeTimer = 0.0f;
    float typeSpeed = 0.08f; // segundos por letra
    int   visibleChars = 0;
    bool  typingDone = false;

    // Efecto parpadeo
    float flickerTimer = 0.0f;
    float flickerAlpha = 1.0f;

    // Estatica
    float staticTimer = 0.0f;
    bool  showStatic = false;
    float staticAlpha = 0.0f;

    // Audio
    bool typeTickPlayed = false;

    // OpenGL
    unsigned int quadShader = 0;
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;
    unsigned int fontTexture = 0;
    stbtt_bakedchar cdata[96];
    bool fontLoaded = false;

    // Helpers
    void compileShader();
    void setupBuffers();
    bool loadFont(const std::string& path);
    void drawRect(float x, float y, float w, float h, float r, float g, float b, float a);
    void drawText(const std::string& text, float x, float y, float scale, float r, float g, float b, float a);
    void drawStatic(float alpha);
    glm::vec2 toNDC(float x, float y);

    // Calcula el ancho de un texto en pixeles
    float textWidth(const std::string& text, float scale);
};