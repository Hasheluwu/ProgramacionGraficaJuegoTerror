#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

struct HUDState
{
    bool hasKey = false;
    bool hasKey2 = false;
    bool hasKey3 = false;
    bool lookingAtItem = false;
    bool lookingAtDoor = false;
    bool doorRequiresKey = false;
    bool doorRequiresKey2 = false;
    bool doorRequiresKey3 = false;
    bool doorIsOpen = false;

    bool lookingAtPuzzleDoor = false;
    bool puzzleDoorBlocked = false;

    // ---- Switch ----
    bool lookingAtSwitch = false;
    bool switchIsOn = false;

    // ---- Ropero ----
    bool lookingAtRopero = false;
    bool roperoOpen = false;
    bool canHideInRopero = false;
    bool playerHiding = false;

    bool  showKeyPickedMsg = false;
    float keyPickedMsgTimer = 0.0f;

    bool  showKey2PickedMsg = false;
    float key2PickedMsgTimer = 0.0f;

    bool  showKey3PickedMsg = false;
    float key3PickedMsgTimer = 0.0f;

    float stamina = 100.0f;
    float staminaMax = 100.0f;
    bool  isExhausted = false;

    int playerLives = 3;   // <-- AÑADIDO
};

class HUD
{
public:
    HUD();
    ~HUD();

    bool Init(const std::string& fontPath, int screenWidth, int screenHeight);
    void Render(const HUDState& state);
    void Resize(int screenWidth, int screenHeight);

private:
    unsigned int shaderProgram = 0;
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int fontTexture = 0;

    static const int BITMAP_W = 512;
    static const int BITMAP_H = 512;
    static const int FIRST_CHAR = 32;
    static const int NUM_CHARS = 96;

    unsigned int rectShaderProgram = 0;
    unsigned int rectVAO = 0;
    unsigned int rectVBO = 0;

    struct CharInfo {
        float x0, y0, x1, y1;
        float xoff, yoff;
        float xadvance;
    };
    CharInfo chars[NUM_CHARS] = {};
    float    fontHeight = 0.0f;

    int  screenW = 1280;
    int  screenH = 720;
    bool ready = false;

    void DrawText(const std::string& text, float x, float y, float scale, glm::vec4 color);
    bool BuildShader();
    bool BuildFont(const std::string& path);
    void UpdateProjection();

    void DrawRect(float x, float y, float w, float h, glm::vec4 color);
    bool BuildRectShader();
};