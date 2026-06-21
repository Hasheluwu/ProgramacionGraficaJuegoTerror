#include "Main.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <SOIL2/SOIL2.h>

// ============================================================
//  SHADERS
// ============================================================

// ---- Quad color solido / fuente ----
static const char* kQuadVS = R"(
#version 330 core
layout(location = 0) in vec4 vertex; // xy=pos, zw=uv
out vec2 TexCoords;
void main() {
    gl_Position = vec4(vertex.xy, 0.0, 1.0);
    TexCoords   = vertex.zw;
}
)";

static const char* kQuadFS = R"(
#version 330 core
in  vec2 TexCoords;
out vec4 FragColor;

uniform vec4      color;
uniform bool      useTexture;
uniform bool      useGradient;
uniform vec4      color2;
uniform sampler2D fontTex;

void main() {
    vec4 base;
    if (useGradient) {
        base = mix(color, color2, TexCoords.y);
    } else if (useTexture) {
        float a = texture(fontTex, TexCoords).r;
        base    = vec4(color.rgb, color.a * a);
    } else {
        base = color;
    }
    FragColor = base;
}
)";

// ---- Imagen RGBA (fondo, logo) ----
static const char* kImgVS = R"(
#version 330 core
layout(location = 0) in vec4 vertex;
out vec2 TexCoords;
void main() {
    gl_Position = vec4(vertex.xy, 0.0, 1.0);
    TexCoords   = vertex.zw;
}
)";

static const char* kImgFS = R"(
#version 330 core
in  vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D imgTex;
uniform float     alpha;
void main() {
    vec4 c    = texture(imgTex, TexCoords);
    FragColor = vec4(c.rgb, c.a * alpha);
}
)";

// ============================================================
//  Compilador de shaders
// ============================================================
static unsigned int compileProgram(const char* vs, const char* fs) {
    int  ok; char log[512];

    unsigned int v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vs, NULL);
    glCompileShader(v);
    glGetShaderiv(v, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glGetShaderInfoLog(v, 512, NULL, log);
        std::cout << "[Menu VS] " << log << "\n";
    }

    unsigned int f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fs, NULL);
    glCompileShader(f);
    glGetShaderiv(f, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        glGetShaderInfoLog(f, 512, NULL, log);
        std::cout << "[Menu FS] " << log << "\n";
    }

    unsigned int p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

// ============================================================
//  Constructor / Destructor
// ============================================================
Menu::Menu(int screenWidth, int screenHeight)
    : scrW(screenWidth), scrH(screenHeight)
{
    compileQuadShader();
    compileImgShader();
    setupQuadBuffers();

    fontLoaded = loadFont("Resources/fonts/RubikDistressed-Regular.ttf");
    if (!fontLoaded)
        std::cout << "[Menu] Fuente no encontrada\n";

    loadImageTexture("Resources/textures/menu_bg.png",
        bgTexture, bgW, bgH);

    loadImageTexture("Resources/textures/university_logo.png",
        logoTexture, logoW, logoH);

    particles.reserve(80);
}

Menu::~Menu() {
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteProgram(quadShader);
    glDeleteProgram(imgShader);
    if (fontTexLarge) glDeleteTextures(1, &fontTexLarge);
    if (fontTexSmall) glDeleteTextures(1, &fontTexSmall);
    if (bgTexture)    glDeleteTextures(1, &bgTexture);
    if (logoTexture)  glDeleteTextures(1, &logoTexture);
}

// ============================================================
//  Compilar shaders
// ============================================================
void Menu::compileQuadShader() { quadShader = compileProgram(kQuadVS, kQuadFS); }
void Menu::compileImgShader() { imgShader = compileProgram(kImgVS, kImgFS); }

void Menu::setupQuadBuffers() {
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// ============================================================
//  Carga de fuente (dos tamanos: 52px titulos, 28px cuerpo)
// ============================================================
bool Menu::loadFont(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<unsigned char> buf(
        (std::istreambuf_iterator<char>(f)), {});

    std::vector<unsigned char> bitmapL(FONT_ATLAS_W * FONT_ATLAS_H);
    stbtt_BakeFontBitmap(buf.data(), 0, 52.0f,
        bitmapL.data(), FONT_ATLAS_W, FONT_ATLAS_H, 32, 96, cdataLarge);

    glGenTextures(1, &fontTexLarge);
    glBindTexture(GL_TEXTURE_2D, fontTexLarge);
    // single-channel bitmap alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
        FONT_ATLAS_W, FONT_ATLAS_H, 0,
        GL_RED, GL_UNSIGNED_BYTE, bitmapL.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<unsigned char> bitmapS(FONT_ATLAS_W * FONT_ATLAS_H);
    stbtt_BakeFontBitmap(buf.data(), 0, 28.0f,
        bitmapS.data(), FONT_ATLAS_W, FONT_ATLAS_H, 32, 96, cdataSmall);

    glGenTextures(1, &fontTexSmall);
    glBindTexture(GL_TEXTURE_2D, fontTexSmall);
    // single-channel bitmap alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
        FONT_ATLAS_W, FONT_ATLAS_H, 0,
        GL_RED, GL_UNSIGNED_BYTE, bitmapS.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return true;
}

// ============================================================
//  Carga imagen con SOIL2
// ============================================================
bool Menu::loadImageTexture(const char* path,
    unsigned int& texOut,
    int& wOut, int& hOut)
{
    texOut = SOIL_load_OGL_texture(
        path,
        SOIL_LOAD_RGBA,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_INVERT_Y
    );

    if (texOut == 0) {
        std::cout << "[Menu] SOIL2 no pudo cargar: " << path
            << " — " << SOIL_last_result() << "\n";
        wOut = hOut = 0;
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, texOut);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &wOut);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &hOut);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

// ============================================================
//  SetLoadingProgress
// ============================================================
void Menu::SetLoadingProgress(float p) { loadingProgress = p; }

// ============================================================
//  toNDC
// ============================================================
glm::vec2 Menu::toNDC(float x, float y) {
    return glm::vec2(
        (x / scrW) * 2.0f - 1.0f,
        1.0f - (y / scrH) * 2.0f);
}

// ============================================================
//  drawRect — quad solido
// ============================================================
void Menu::drawRect(float x, float y, float w, float h,
    float r, float g, float b, float a)
{
    glm::vec2 tl = toNDC(x, y);
    glm::vec2 br = toNDC(x + w, y + h);

    float verts[6][4] = {
        {tl.x,tl.y,0,0},{br.x,tl.y,1,0},{br.x,br.y,1,1},
        {tl.x,tl.y,0,0},{br.x,br.y,1,1},{tl.x,br.y,0,1}
    };

    glUseProgram(quadShader);
    glUniform4f(glGetUniformLocation(quadShader, "color"), r, g, b, a);
    glUniform1i(glGetUniformLocation(quadShader, "useTexture"), 0);
    glUniform1i(glGetUniformLocation(quadShader, "useGradient"), 0);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ============================================================
//  drawRectGradient — quad con degradado vertical
// ============================================================
void Menu::drawRectGradient(float x, float y, float w, float h,
    float r0, float g0, float b0, float a0,
    float r1, float g1, float b1, float a1)
{
    glm::vec2 tl = toNDC(x, y);
    glm::vec2 br = toNDC(x + w, y + h);

    float verts[6][4] = {
        {tl.x,tl.y,0,0},{br.x,tl.y,1,0},{br.x,br.y,1,1},
        {tl.x,tl.y,0,0},{br.x,br.y,1,1},{tl.x,br.y,0,1}
    };

    glUseProgram(quadShader);
    glUniform4f(glGetUniformLocation(quadShader, "color"), r0, g0, b0, a0);
    glUniform4f(glGetUniformLocation(quadShader, "color2"), r1, g1, b1, a1);
    glUniform1i(glGetUniformLocation(quadShader, "useTexture"), 0);
    glUniform1i(glGetUniformLocation(quadShader, "useGradient"), 1);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ============================================================
//  drawImage — textura RGBA completa
// ============================================================
void Menu::drawImage(unsigned int tex,
    float x, float y, float w, float h,
    float alpha)
{
    if (!tex) return;

    glm::vec2 tl = toNDC(x, y);
    glm::vec2 br = toNDC(x + w, y + h);

    // UV invertido en Y porque SOIL2 usa SOIL_FLAG_INVERT_Y
    float verts[6][4] = {
        {tl.x,tl.y,0,1},{br.x,tl.y,1,1},{br.x,br.y,1,0},
        {tl.x,tl.y,0,1},{br.x,br.y,1,0},{tl.x,br.y,0,0}
    };

    glUseProgram(imgShader);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(glGetUniformLocation(imgShader, "imgTex"), 0);
    glUniform1f(glGetUniformLocation(imgShader, "alpha"), alpha);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ============================================================
//  drawText
// ============================================================
void Menu::drawText(const std::string& text,
    float x, float y, float scale,
    float r, float g, float b, float a,
    bool isLarge)
{
    if (!fontLoaded || text.empty()) return;

    unsigned int     tex = isLarge ? fontTexLarge : fontTexSmall;
    stbtt_bakedchar* cd = isLarge ? cdataLarge : cdataSmall;

    glUseProgram(quadShader);
    glUniform4f(glGetUniformLocation(quadShader, "color"), r, g, b, a);
    glUniform1i(glGetUniformLocation(quadShader, "useTexture"), 1);
    glUniform1i(glGetUniformLocation(quadShader, "useGradient"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(glGetUniformLocation(quadShader, "fontTex"), 0);

    float cx = x, cy = y;
    for (char c : text) {
        if (c < 32 || c > 126) continue;
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(cd, FONT_ATLAS_W, FONT_ATLAS_H,
            c - 32, &cx, &cy, &q, 1);

        float qx0 = x + (q.x0 - x) * scale;
        float qx1 = x + (q.x1 - x) * scale;
        float qy0 = y + (q.y0 - y) * scale;
        float qy1 = y + (q.y1 - y) * scale;

        glm::vec2 tl = toNDC(qx0, qy0);
        glm::vec2 br = toNDC(qx1, qy1);

        float verts[6][4] = {
            {tl.x,tl.y,q.s0,q.t0},{br.x,tl.y,q.s1,q.t0},{br.x,br.y,q.s1,q.t1},
            {tl.x,tl.y,q.s0,q.t0},{br.x,br.y,q.s1,q.t1},{tl.x,br.y,q.s0,q.t1}
        };

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
}

// ============================================================
//  measureText — ancho aproximado en pixeles
// ============================================================
float Menu::measureText(const std::string& text,
    float scale, bool isLarge)
{
    stbtt_bakedchar* cd = isLarge ? cdataLarge : cdataSmall;
    float cx = 0.0f, cy = 0.0f;
    for (char c : text) {
        if (c < 32 || c > 126) continue;
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(cd, FONT_ATLAS_W, FONT_ATLAS_H,
            c - 32, &cx, &cy, &q, 1);
    }
    return cx * scale;
}

// ============================================================
//  drawSlider
// ============================================================
void Menu::drawSlider(float x, float y, float w, float h,
    float value, bool selected)
{
    // Track
    drawRect(x, y, w, h, 0.12f, 0.0f, 0.0f, 1.0f);

    // Fill
    float fillW = w * glm::clamp(value, 0.0f, 1.0f);
    if (fillW > 2.0f) {
        if (selected)
            drawRectGradient(x, y, fillW, h,
                0.9f, 0.05f, 0.05f, 1.0f,
                0.5f, 0.0f, 0.0f, 1.0f);
        else
            drawRect(x, y, fillW, h, 0.55f, 0.0f, 0.0f, 1.0f);
    }

    // Handle
    float hx = x + fillW - 5.0f;
    drawRect(hx, y - 4.0f, 10.0f, h + 8.0f,
        selected ? 1.0f : 0.7f, 0.1f, 0.1f, 1.0f);

    // Borde exterior
    drawRect(x - 1, y - 1, w + 2, 1, 0.35f, 0.0f, 0.0f, 1.0f);
    drawRect(x - 1, y + h, w + 2, 1, 0.35f, 0.0f, 0.0f, 1.0f);
    drawRect(x - 1, y - 1, 1, h + 2, 0.35f, 0.0f, 0.0f, 1.0f);
    drawRect(x + w, y - 1, 1, h + 2, 0.35f, 0.0f, 0.0f, 1.0f);
}

// ============================================================
//  Fade helpers
// ============================================================
void Menu::startFade(MenuState next, MenuAction action) {
    if (fadingOut) return;
    fadingOut = true;
    fadingIn = false;
    pendingState = next;
    pendingAction = action;
    transTimer = 0.0f;
}

void Menu::updateFade(float dt) {
    const float SPEED = 3.0f;
    if (fadingOut) {
        fadeAlpha += dt * SPEED;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f;
            fadingOut = false;
            fadingIn = true;
            previousState = state;
            state = pendingState;
            transTimer = 0.0f;
            if (state == MenuState::CREDITS) creditsScroll = 0.0f;
        }
    }
    else if (fadingIn) {
        fadeAlpha -= dt * SPEED;
        if (fadeAlpha <= 0.0f) {
            fadeAlpha = 0.0f;
            fadingIn = false;
        }
    }
}

void Menu::renderFadeOverlay() {
    if (fadeAlpha <= 0.0f) return;
    drawRect(0, 0, (float)scrW, (float)scrH,
        0.0f, 0.0f, 0.0f, fadeAlpha);
}

// ============================================================
//  Particulas
// ============================================================
void Menu::spawnParticle() {
    Particle p;
    p.x = (float)(rand() % scrW);
    p.y = (float)scrH + 5.0f;
    p.vx = ((float)(rand() % 100) - 50.0f) * 0.01f;
    p.vy = -((float)(rand() % 40) + 20.0f) * 0.3f;
    p.maxLife = 3.0f + (rand() % 300) * 0.01f;
    p.life = p.maxLife;
    p.size = 2.0f + (rand() % 4);
    p.alpha = 0.0f;
    particles.push_back(p);
}

void Menu::updateParticles(float dt) {
    static float spawnAcc = 0.0f;
    spawnAcc += dt;
    if (spawnAcc > 0.08f && particles.size() < 80) {
        spawnParticle();
        spawnAcc = 0.0f;
    }
    for (auto& p : particles) {
        p.x += p.vx * dt * 60.0f;
        p.y += p.vy * dt;
        p.life -= dt;
        float t = 1.0f - p.life / p.maxLife;
        p.alpha = (t < 0.1f) ? t / 0.1f : (t > 0.8f ? (1.0f - t) / 0.2f : 1.0f);
        p.alpha *= 0.45f;
    }
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return p.life <= 0.0f; }),
        particles.end());
}

void Menu::renderParticles() {
    for (const auto& p : particles) {
        drawRect(p.x - p.size * 0.5f,
            p.y - p.size * 0.5f,
            p.size, p.size,
            0.85f, 0.05f, 0.05f, p.alpha);
    }
}

// ============================================================
//  UPDATE principal
// ============================================================
MenuAction Menu::Update(GLFWwindow* window, float deltaTime) {
    blinkTimer += deltaTime;
    hoverTimer += deltaTime;
    updateFade(deltaTime);
    updateParticles(deltaTime);

    bool clickNow = glfwGetMouseButton(window,
        GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool upNow = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
    bool downNow = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
    bool leftNow = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
    bool rightNow = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    bool enterNow = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    // ---- MAIN ----
    if (state == MenuState::MAIN) {
        // Keyboard navigation
        if (upNow && !prevUp) selectedItem = std::max(0, selectedItem - 1);
        if (downNow && !prevDown) selectedItem = std::min(3, selectedItem + 1);
        if (enterNow && !prevEnterKey) {
            // Activate selected
            if (selectedItem == 0) startFade(MenuState::LOADING, MenuAction::START_GAME);
            else if (selectedItem == 1) startFade(MenuState::SETTINGS, MenuAction::NONE);
            else if (selectedItem == 2) startFade(MenuState::CREDITS, MenuAction::NONE);
            else if (selectedItem == 3) return MenuAction::QUIT;
        }

        glfwGetCursorPos(window, &mouseX, &mouseY);

        float tx = ((float)mouseX / scrW - 0.5f) * 18.0f;
        float ty = ((float)mouseY / scrH - 0.5f) * 10.0f;
        parallaxX += (tx - parallaxX) * 0.05f;
        parallaxY += (ty - parallaxY) * 0.05f;

        // Mouse hover overrides keyboard selection
        hoveredItem = -1;
        for (int i = 0; i < 4; i++) {
            float yPos = scrH * 0.42f + i * 72.0f;
            float x0 = scrW * 0.33f, y0 = yPos - 10;
            float x1 = x0 + scrW * 0.34f, y1 = y0 + 52.0f;
            if (mouseX >= x0 && mouseX <= x1 && mouseY >= y0 && mouseY <= y1)
                hoveredItem = i;
        }
        if (hoveredItem != -1) selectedItem = hoveredItem;

        if (clickNow && !prevEnter && hoveredItem != -1) {
            prevEnter = true;
            if (selectedItem == 0) startFade(MenuState::LOADING, MenuAction::START_GAME);
            else if (selectedItem == 1) startFade(MenuState::SETTINGS, MenuAction::NONE);
            else if (selectedItem == 2) startFade(MenuState::CREDITS, MenuAction::NONE);
            else if (selectedItem == 3) return MenuAction::QUIT;
        }
    }

    // ---- SETTINGS ----
    else if (state == MenuState::SETTINGS) {
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // keyboard navigation between sliders
        if (upNow && !prevUp) settingsSelected = glm::clamp(settingsSelected - 1, 0, 2);
        if (downNow && !prevDown) settingsSelected = glm::clamp(settingsSelected + 1, 0, 2);

        // adjust currently selected slider with left/right (continuous while held)
        float speed = 0.75f * deltaTime; // slightly faster for responsiveness
        if (settingsSelected == 0 && leftNow)
            mouseSensitivity = glm::clamp(mouseSensitivity - speed * 0.3f, 0.01f, 0.30f);
        if (settingsSelected == 0 && rightNow)
            mouseSensitivity = glm::clamp(mouseSensitivity + speed * 0.3f, 0.01f, 0.30f);
        if (settingsSelected == 1 && leftNow)
            brightness = glm::clamp(brightness - speed * 1.5f, 0.3f, 2.0f);
        if (settingsSelected == 1 && rightNow)
            brightness = glm::clamp(brightness + speed * 1.5f, 0.3f, 2.0f);
        if (settingsSelected == 2 && leftNow)
            masterVolume = glm::clamp(masterVolume - speed, 0.0f, 1.0f);
        if (settingsSelected == 2 && rightNow)
            masterVolume = glm::clamp(masterVolume + speed, 0.0f, 1.0f);

        // Mouse interaction con sliders
        struct SliderRect { float x, y, w, h; };
        float bx = scrW * 0.25f, bw = scrW * 0.50f, bh = 18.0f;
        SliderRect sliders[3] = {
            {bx, scrH * 0.40f, bw, bh},
            {bx, scrH * 0.55f, bw, bh},
            {bx, scrH * 0.70f, bw, bh}
        };

        // Si no estamos arrastrando, detectar hover y click inicial
        if (sliderDragging == -1) {
            for (int i = 0; i < 3; i++) {
                auto& s = sliders[i];
                if (mouseX >= s.x && mouseX <= s.x + s.w &&
                    mouseY >= s.y && mouseY <= s.y + s.h)
                {
                    settingsSelected = i;
                    // Iniciar arrastre si se hace click
                    if (clickNow) {
                        sliderDragging = i;
                    }
                }
            }
        }
        // Si estamos arrastrando, actualizar el valor del slider continuamente
        else if (sliderDragging >= 0 && sliderDragging < 3) {
            auto& s = sliders[sliderDragging];
            float v = glm::clamp((float)(mouseX - s.x) / s.w, 0.0f, 1.0f);

            if (sliderDragging == 0)
                mouseSensitivity = glm::clamp(v * 0.29f + 0.01f, 0.01f, 0.30f);
            else if (sliderDragging == 1)
                brightness = glm::clamp(v * 1.7f + 0.3f, 0.3f, 2.0f);
            else if (sliderDragging == 2)
                masterVolume = glm::clamp(v, 0.0f, 1.0f);

            // Soltar el slider cuando se suelta el click
            if (!clickNow) {
                sliderDragging = -1;
            }
        }

        // Boton BACK con mouse o teclado — vuelve al estado anterior
        float backX = scrW * 0.5f - 70, backY = scrH * 0.88f;
        bool backHov = mouseX >= backX && mouseX <= backX + 140 &&
            mouseY >= backY && mouseY <= backY + 40;
        if ((backHov && clickNow && !prevEnter) || (enterNow && !prevEnterKey && backHov))
            startFade(previousState);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            startFade(previousState);
    }

    // ---- CREDITS ----
    else if (state == MenuState::CREDITS) {
        sliderDragging = -1;  // resetear arrastre si estábamos en settings
        creditsScroll += deltaTime * 22.0f;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            startFade(MenuState::MAIN);
        if (clickNow && !prevEnter)
            startFade(MenuState::MAIN);
    }

    // ---- LOADING ----
    else if (state == MenuState::LOADING) {
        if (loadingProgress >= 1.0f) {
            state = MenuState::PLAYING;
            prevEnter = clickNow;
            return MenuAction::START_GAME;
        }
    }

    // ---- PAUSED ----
    else if (state == MenuState::PAUSED) {
        // keyboard navigation
        if (upNow && !prevUp) selectedItem = std::max(0, selectedItem - 1);
        if (downNow && !prevDown) selectedItem = std::min(4, selectedItem + 1);
        if (enterNow && !prevEnterKey) {
            switch (selectedItem) {
            case 0: state = MenuState::PLAYING; return MenuAction::RESUME_GAME;
            case 1: startFade(MenuState::LOADING, MenuAction::RESTART_GAME); break;
            case 2: startFade(MenuState::SETTINGS, MenuAction::NONE); break;
            case 3: startFade(MenuState::MAIN, MenuAction::GO_TO_MAIN); break;
            case 4: return MenuAction::QUIT;
            }
        }

        glfwGetCursorPos(window, &mouseX, &mouseY);

        const int NUM_PAUSE = 5;
        hoveredItem = -1;
        for (int i = 0; i < NUM_PAUSE; i++) {
            float yPos = scrH * 0.38f + i * 60.0f;
            float x0 = scrW * 0.35f, y0 = yPos - 8;
            float x1 = x0 + scrW * 0.30f, y1 = y0 + 44.0f;
            if (mouseX >= x0 && mouseX <= x1 && mouseY >= y0 && mouseY <= y1)
                hoveredItem = i;
        }
        if (hoveredItem != -1) selectedItem = hoveredItem;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            state = MenuState::PLAYING;
            prevEnter = clickNow;
            return MenuAction::RESUME_GAME;
        }

        if (clickNow && !prevEnter && hoveredItem != -1) {
            prevEnter = true;
            switch (selectedItem) {
            case 0: state = MenuState::PLAYING; return MenuAction::RESUME_GAME;
            case 1: startFade(MenuState::LOADING, MenuAction::RESTART_GAME); break;
            case 2: startFade(MenuState::SETTINGS, MenuAction::NONE); break;
            case 3: startFade(MenuState::MAIN, MenuAction::GO_TO_MAIN); break;
            case 4: return MenuAction::QUIT;
            }
        }
    }

    // ---- WIN ----
    else if (state == MenuState::WIN) {
        if (enterNow && !prevEnterKey) {
            startFade(MenuState::MAIN, MenuAction::GO_TO_MAIN);
        }
        if (clickNow && !prevEnter) {
            startFade(MenuState::MAIN, MenuAction::GO_TO_MAIN);
        }
    }

    // update previous-input flags for edge detection (DEBE ESTAR AQUÍ, FUERA DE TODOS LOS ESTADOS)
    prevEnter = clickNow;
    prevUp = upNow;
    prevDown = downNow;
    prevLeft = leftNow;
    prevRight = rightNow;
    prevEnterKey = enterNow;

    // Check if fade has completed and we have a pending action to return
    if (!fadingOut && !fadingIn && fadeAlpha <= 0.0f && pendingAction != MenuAction::NONE) {
        MenuAction result = pendingAction;
        pendingAction = MenuAction::NONE;
        return result;
    }

    return MenuAction::NONE;
}

// ============================================================
//  RENDER principal
// ============================================================
void Menu::Render() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawRect(0, 0, (float)scrW, (float)scrH, 0.0f, 0.0f, 0.0f, 1.0f);

    switch (state) {
    case MenuState::MAIN:     renderMain();            break;
    case MenuState::SETTINGS: renderSettings(nullptr); break;
    case MenuState::CREDITS:  renderCredits();         break;
    case MenuState::LOADING:  renderLoading();         break;
    case MenuState::PAUSED:   renderPause();           break;
    case MenuState::WIN:      renderWin();             break;   // <-- nuevo
    default: break;
    }

    renderFadeOverlay();
    glEnable(GL_DEPTH_TEST);
}

// ============================================================
//  renderMain
// ============================================================
void Menu::renderMain() {
    // Fondo con parallax
    if (bgTexture) {
        drawImage(bgTexture,
            -parallaxX - 20.0f, -parallaxY - 20.0f,
            (float)scrW + 40.0f,
            (float)scrH + 40.0f,
            0.72f);
    }

    // Vinetas oscuras en bordes
    drawRectGradient(0, 0, (float)scrW, (float)scrH * 0.30f,
        0.0f, 0.0f, 0.0f, 0.75f, 0.0f, 0.0f, 0.0f, 0.0f);
    drawRectGradient(0, (float)scrH * 0.70f, (float)scrW, (float)scrH * 0.30f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.85f);

    renderParticles();

    // Titulo HUNTED
    float blink = 0.75f + 0.25f * sinf(blinkTimer * 2.8f);
    float titleW = measureText("HUNTED", 1.0f, true);
    float titleX = (scrW - titleW) * 0.5f + parallaxX * 0.3f;
    float titleY = scrH * 0.20f;

    // Sombra
    drawText("HUNTED", titleX + 3, titleY + 3, 1.0f,
        0.0f, 0.0f, 0.0f, 0.6f, true);
    // Titulo
    drawText("HUNTED", titleX, titleY, 1.0f,
        blink, 0.05f, 0.05f, 1.0f, true);

    // Linea separadora con degradado desde el centro
    float lineY = scrH * 0.30f;
    float lineW = scrW * 0.25f;
    float lineX = scrW * 0.25f;
    drawRectGradient(lineX, lineY, lineW, 2.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.0f, 0.0f, 1.0f);
    drawRectGradient(lineX + lineW, lineY, lineW, 2.0f,
        0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    // Items del menu
    const char* items[] = { "PLAY", "SETTINGS", "CREDITS", "QUIT" };
    for (int i = 0; i < 4; i++) {
        float yPos = scrH * 0.42f + i * 72.0f;
        bool  sel = (i == selectedItem || i == hoveredItem);
        float itemW = measureText(items[i], 1.0f, true);
        float itemX = (scrW - itemW) * 0.5f;

        if (sel) {
            float pulse = 0.12f + 0.06f * sinf(blinkTimer * 5.0f);
            drawRectGradient(scrW * 0.33f, yPos - 12, scrW * 0.34f, 52.0f,
                pulse, 0.0f, 0.0f, 0.85f,
                0.0f, 0.0f, 0.0f, 0.0f);

            // Acento izquierdo
            drawRect(scrW * 0.33f, yPos - 12, 3.0f, 52.0f,
                0.9f, 0.0f, 0.0f, 1.0f);

            // Glow del texto
            for (int g = 3; g >= 1; g--) {
                drawText(items[i], itemX + g, yPos + 38 + g, 1.0f,
                    0.9f, 0.0f, 0.0f, 0.08f * g, true);
            }

            float rb = 0.88f + 0.12f * sinf(blinkTimer * 6.0f);
            drawText(items[i], itemX, yPos + 38, 1.0f,
                rb, 0.15f, 0.15f, 1.0f, true);
        }
        else {
            drawText(items[i], itemX, yPos + 38, 1.0f,
                0.38f, 0.38f, 0.38f, 1.0f, true);
        }
    }

    // Pie de pantalla
    std::string footer = "v0.1 ALPHA";
    float fw = measureText(footer, 0.55f, false);
    drawText(footer, (scrW - fw) * 0.5f, (float)scrH - 18.0f,
        0.55f, 0.25f, 0.0f, 0.0f, 0.7f, false);
}

// ============================================================
//  renderSettings  (publica para compatibilidad con Main.cpp)
// ============================================================
void Menu::renderSettings(GLFWwindow* /*window*/) {
    drawRect(0, 0, (float)scrW, (float)scrH, 0.0f, 0.0f, 0.0f, 0.88f);

    // Panel central
    float px = scrW * 0.18f, py = scrH * 0.08f;
    float pw = scrW * 0.64f, ph = scrH * 0.84f;
    drawRectGradient(px, py, pw, ph,
        0.05f, 0.0f, 0.0f, 0.95f,
        0.0f, 0.0f, 0.0f, 0.95f);
    drawRect(px, py, pw, 2, 0.5f, 0.0f, 0.0f, 1.0f);
    drawRect(px, py + ph - 2, pw, 2, 0.5f, 0.0f, 0.0f, 1.0f);
    drawRect(px, py, 2, ph, 0.5f, 0.0f, 0.0f, 1.0f);
    drawRect(px + pw - 2, py, 2, ph, 0.5f, 0.0f, 0.0f, 1.0f);

    // Titulo
    float tw = measureText("SETTINGS", 1.0f, true);
    drawText("SETTINGS", (scrW - tw) * 0.5f, scrH * 0.16f, 1.0f,
        0.85f, 0.05f, 0.05f, 1.0f, true);
    drawRect(scrW * 0.25f, scrH * 0.24f, scrW * 0.50f, 1.5f, 0.4f, 0.0f, 0.0f, 1.0f);

    // Sliders
    struct SliderData { const char* label; float value; float minV; float maxV; };
    SliderData sdata[3] = {
        {"MOUSE SENSITIVITY", (mouseSensitivity - 0.01f) / (0.30f - 0.01f), 0.01f, 0.30f},
        {"BRIGHTNESS",        (brightness - 0.3f) / (2.0f - 0.3f),          0.3f,  2.0f },
        {"MASTER VOLUME",     masterVolume,                            0.0f,  1.0f }
    };

    float rowY[3] = { scrH * 0.33f, scrH * 0.50f, scrH * 0.67f };
    float bx = scrW * 0.25f, bw = scrW * 0.50f, bh = 18.0f;

    for (int i = 0; i < 3; i++) {
        bool sel = (settingsSelected == i);
        float lw = measureText(sdata[i].label, 0.65f, false);
        drawText(sdata[i].label, (scrW - lw) * 0.5f, rowY[i],
            0.65f,
            sel ? 0.95f : 0.60f,
            0.10f,
            0.10f,
            1.0f, false);

        drawSlider(bx, rowY[i] + 28.0f, bw, bh, sdata[i].value, sel);

        // Valor numerico
        char buf[16];
        float realVal = sdata[i].minV + sdata[i].value * (sdata[i].maxV - sdata[i].minV);
        snprintf(buf, sizeof(buf), "%.2f", realVal);
        float vw = measureText(buf, 0.60f, false);
        drawText(buf, (scrW - vw) * 0.5f, rowY[i] + 62.0f,
            0.60f, sel ? 1.0f : 0.5f, 0.2f, 0.2f, 1.0f, false);
    }

    // Boton BACK
    float backX = scrW * 0.5f - 70, backY = scrH * 0.88f;
    bool backHov = mouseX >= backX && mouseX <= backX + 140 &&
        mouseY >= backY && mouseY <= backY + 40;
    drawRect(backX, backY, 140, 40,
        backHov ? 0.35f : 0.12f,
        0.0f, 0.0f,
        backHov ? 1.0f : 0.85f);
    float bkw = measureText("BACK", 0.70f, false);
    drawText("BACK", backX + (140 - bkw) * 0.5f, backY + 30,
        0.70f, 1.0f, 0.3f, 0.3f, 1.0f, false);

    // Hint
    std::string hint = "< > to adjust   |   UP/DOWN to select   |   ESC to go back";
    float hw = measureText(hint, 0.45f, false);
    drawText(hint, (scrW - hw) * 0.5f, scrH * 0.96f,
        0.45f, 0.30f, 0.0f, 0.0f, 0.75f, false);
}

// ============================================================
//  renderCredits
// ============================================================
void Menu::renderCredits() {
    drawRect(0, 0, (float)scrW, (float)scrH, 0.0f, 0.0f, 0.0f, 0.92f);

    float cy = scrH * 0.12f - creditsScroll;

    auto line = [&](const std::string& txt, float scale,
        float r, float g, float b, bool large = false)
        {
            float w = measureText(txt, scale, large);
            float alpha = 1.0f;
            const float fade = 60.0f;
            if (cy < fade)      alpha = glm::clamp(cy / fade, 0.0f, 1.0f);
            if (cy > scrH - fade) alpha = glm::clamp((scrH - cy) / fade, 0.0f, 1.0f);
            if (cy > -80.0f && cy < scrH + 80.0f)
                drawText(txt, (scrW - w) * 0.5f, cy, scale, r, g, b, alpha, large);
            cy += scale * 60.0f + 8.0f;
        };
    auto gap = [&](float px) { cy += px; };

    line("UNIVERSITY", 0.80f, 0.7f, 0.0f, 0.0f, true);
    gap(8.0f);

    // Logo
    if (logoTexture) {
        float lw = 160.0f;
        float lh = 160.0f * ((float)logoH / glm::max(logoW, 1));
        float lx = (scrW - lw) * 0.5f;
        float fa = 1.0f;
        const float fade = 60.0f;
        if (cy < fade)      fa = glm::clamp(cy / fade, 0.0f, 1.0f);
        if (cy > scrH - fade) fa = glm::clamp((scrH - cy) / fade, 0.0f, 1.0f);
        if (cy > -lh && cy < scrH + lh)
            drawImage(logoTexture, lx, cy, lw, lh, fa);
        cy += lh + 16.0f;
    }

    gap(20.0f);
    line("PROJECT", 0.75f, 0.65f, 0.0f, 0.0f, true);
    line("HUNTED", 0.90f, 0.90f, 0.05f, 0.05f, true);
    gap(20.0f);

    line("AUTHORS", 0.75f, 0.65f, 0.0f, 0.0f, true);
    const char* authors[] = {
        "Navas Jorge Emilio",
        "Guido Torrez Maximiliano",
        "Lopez Arguello Hashel Ignacio",
        "Garcia Telleria Luis Angel"
    };
    for (auto* a : authors)
        line(a, 0.60f, 0.55f, 0.55f, 0.55f, false);
    gap(20.0f);

    line("TECNOLOGIES", 0.75f, 0.65f, 0.0f, 0.0f, true);
    const char* techs[] = {
        "OpenGL","GLFW","GLAD","ASSIMP","stb_truetype","SOIL2","GLM"
    };
    for (auto* t : techs)
        line(t, 0.60f, 0.55f, 0.55f, 0.55f, false);

    gap(40.0f);
    line("ESC  /  CLICK  to return", 0.55f, 0.28f, 0.0f, 0.0f, false);

    // Reset scroll al llegar al final
    if (creditsScroll > cy + scrH * 0.5f)
        creditsScroll = 0.0f;

    // Vinetas superior e inferior
    drawRectGradient(0, 0, (float)scrW, 80.0f,
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    drawRectGradient(0, (float)scrH - 80, (float)scrW, 80.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

// ============================================================
//  renderLoading - Pantalla de carga mejorada
// ============================================================
void Menu::renderLoading() {
    // Fondo degradado oscuro profesional
    drawRectGradient(0, 0, (float)scrW, (float)scrH,
        0.02f, 0.02f, 0.03f, 1.0f,
        0.05f, 0.01f, 0.02f, 1.0f);

    // Partículas de fondo (efecto ambiental)
    renderParticles();

    // Viñetas superior e inferior para profundidad
    drawRectGradient(0, 0, (float)scrW, (float)scrH * 0.25f,
        0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f);
    drawRectGradient(0, (float)scrH * 0.75f, (float)scrW, (float)scrH * 0.25f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f);

    // Línea decorativa superior
    drawRect(0, scrH * 0.30f, (float)scrW, 1.0f, 0.8f, 0.0f, 0.0f, 0.3f);

    // Línea decorativa inferior
    drawRect(0, scrH * 0.65f, (float)scrW, 1.0f, 0.8f, 0.0f, 0.0f, 0.3f);

    // Subtítulo tenue del juego - mejorado
    float blink = 0.55f + 0.45f * sinf(blinkTimer * 3.5f);
    float hw2 = measureText("HUNTED", 0.85f, true);

    // Efecto glow del subtítulo
    for (int g = 4; g >= 1; g--) {
        drawText("HUNTED", (scrW - hw2) * 0.5f + g * 0.5f, scrH * 0.20f,
            0.85f, blink * 0.15f, 0.0f, 0.0f, 0.15f / g, true);
    }
    drawText("HUNTED", (scrW - hw2) * 0.5f, scrH * 0.20f,
        0.85f, blink * 0.4f, 0.0f, 0.0f, 0.65f, true);

    // Texto LOADING... - con animación mejorada
    float lw = measureText("LOADING", 1.0f, true);
    float pulse = 0.8f + 0.2f * sinf(blinkTimer * 4.0f);

    drawText("LOADING", (scrW - lw) * 0.5f, scrH * 0.38f,
        1.0f, pulse, 0.05f, 0.05f, 1.0f, true);

    // Puntos animados
    float dotAlpha = sinf(blinkTimer * 6.0f) * 0.5f + 0.5f;
    float dotOffset = 0.0f;
    for (int i = 0; i < 3; i++) {
        float dotBlink = sinf(blinkTimer * 6.0f + i * 1.05f) * 0.5f + 0.5f;
        drawText(".", (scrW - lw) * 0.5f + lw + 15 + i * 20, scrH * 0.38f,
            1.0f, pulse, 0.05f, 0.05f, dotBlink, true);
    }

    // Barra de progreso - completamente mejorada
    float barX = scrW * 0.15f, barY = scrH * 0.56f;
    float barW = scrW * 0.70f, barH = 28.0f;

    // Fondo de la barra con efecto 3D
    drawRectGradient(barX - 3, barY - 3, barW + 6, barH + 6,
        0.15f, 0.0f, 0.0f, 1.0f,
        0.08f, 0.0f, 0.0f, 1.0f);

    // Border interior
    drawRect(barX - 1, barY - 1, barW + 2, barH + 2, 0.35f, 0.0f, 0.0f, 0.6f);

    // Track oscuro
    drawRectGradient(barX, barY, barW, barH,
        0.08f, 0.0f, 0.0f, 1.0f,
        0.05f, 0.0f, 0.0f, 1.0f);

    float fillW = barW * loadingProgress;
    if (fillW > 2.0f) {
        // Fill con degradado dinámico
        float fillGlow = 0.3f + 0.2f * sinf(blinkTimer * 3.0f);
        drawRectGradient(barX, barY, fillW, barH,
            0.25f + fillGlow * 0.1f, 0.0f, 0.0f, 1.0f,
            0.40f + fillGlow * 0.15f, 0.05f, 0.0f, 1.0f);

        // Highlight superior (efecto 3D)
        drawRect(barX, barY, fillW, barH * 0.3f,
            0.75f, 0.1f, 0.1f, 0.4f);

        // Glow en el extremo derecho - mejorado
        float gw = glm::min(fillW, 40.0f);
        drawRectGradient(barX + fillW - gw, barY, gw, barH,
            0.0f, 0.0f, 0.0f, 0.0f,
            1.0f, 0.15f, 0.1f, 0.8f);

        // Gotas de sangre animadas - mejoradas
        for (int d = 0; d < 5; d++) {
            float speed = 55.0f + d * 18.0f;
            float period = 75.0f + d * 22.0f;
            float offset = fmodf(blinkTimer * speed + d * 47.0f, period);
            float dropX = barX + fillW - 3.0f - d * 6.0f;
            float dropY = barY + barH + offset;
            float alpha = glm::clamp(1.0f - offset / period, 0.0f, 1.0f);
            float bright = 0.6f + 0.4f * (1.0f - offset / period);
            float dW = 6.0f - d * 0.4f;
            float dH = 10.0f + d * 4.0f;

            // Cuerpo de la gota
            drawRect(dropX, dropY, dW, dH * 0.6f,
                bright, 0.02f, 0.0f, alpha);

            // Punta de la gota (triangular approximation)
            drawRect(dropX + dW * 0.2f, dropY + dH * 0.6f, dW * 0.6f, dH * 0.4f,
                bright * 0.8f, 0.0f, 0.0f, alpha * 0.8f);

            // Salpicadura cuando toca el suelo
            if (offset > period * 0.80f) {
                float splashProgress = (offset - period * 0.80f) / (period * 0.20f);
                float sp = splashProgress * 20.0f;
                float splashAlpha = (1.0f - splashProgress) * alpha * 0.6f;
                drawRect(dropX - sp * 0.5f, dropY + dH, dW + sp, 2.0f,
                    0.45f, 0.0f, 0.0f, splashAlpha);
            }
        }
    }

    // Porcentaje - mejorado visualmente
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", (int)(loadingProgress * 100));
    float pw = measureText(buf, 0.80f, false);

    // Sombra del porcentaje
    drawText(buf, (scrW - pw) * 0.5f + 2, scrH * 0.73f + 2,
        0.80f, 0.0f, 0.0f, 0.0f, 0.4f, false);

    // Texto del porcentaje
    drawText(buf, (scrW - pw) * 0.5f, scrH * 0.73f,
        0.80f, 0.95f, 0.1f, 0.05f, 1.0f, false);

    // Hint animado - mejorado
    std::string hint = "Preparing the hunt...";
    float hintW = measureText(hint, 0.50f, false);
    float hintBlink = 0.4f + 0.3f * sinf(blinkTimer * 1.5f);

    drawText(hint, (scrW - hintW) * 0.5f, scrH * 0.88f,
        0.50f, 0.35f, 0.05f, 0.02f, hintBlink, false);

    // Líneas decorativas laterales
    float lineStartX = scrW * 0.10f;
    float lineEndX = scrW * 0.90f;
    float lineY = scrH * 0.86f;

    drawRect(lineStartX, lineY, (scrW * 0.30f - 50), 1.0f,
        0.6f, 0.0f, 0.0f, 0.4f);
    drawRect(lineEndX - (scrW * 0.30f - 50), lineY, (scrW * 0.30f - 50), 1.0f,
        0.6f, 0.0f, 0.0f, 0.4f);
}

// ============================================================
//  renderPause
// ============================================================
void Menu::renderPause() {
    // Overlay oscuro sobre la escena del juego
    drawRect(0, 0, (float)scrW, (float)scrH, 0.0f, 0.0f, 0.0f, 0.72f);

    // Panel central
    float pw = scrW * 0.38f, ph = scrH * 0.60f;
    float px = (scrW - pw) * 0.5f, py = (scrH - ph) * 0.5f;
    drawRectGradient(px, py, pw, ph,
        0.06f, 0.0f, 0.0f, 0.97f,
        0.0f, 0.0f, 0.0f, 0.97f);
    drawRect(px, py, pw, 2, 0.55f, 0.0f, 0.0f, 1.0f);
    drawRect(px, py + ph - 2, pw, 2, 0.55f, 0.0f, 0.0f, 1.0f);
    drawRect(px, py, 2, ph, 0.55f, 0.0f, 0.0f, 1.0f);
    drawRect(px + pw - 2, py, 2, ph, 0.55f, 0.0f, 0.0f, 1.0f);

    // Titulo PAUSED
    float tw = measureText("PAUSED", 0.90f, true);
    drawText("PAUSED", (scrW - tw) * 0.5f, py + 52.0f,
        0.90f, 0.82f, 0.02f, 0.02f, 1.0f, true);
    drawRect(px + 20, py + 68, pw - 40, 1.5f, 0.4f, 0.0f, 0.0f, 1.0f);

    const char* items[] = {
        "CONTINUE","RESTART","SETTINGS","MAIN MENU","QUIT"
    };
    for (int i = 0; i < 5; i++) {
        float yPos = scrH * 0.38f + i * 60.0f;
        bool  sel = (i == selectedItem || i == hoveredItem);
        float iw = measureText(items[i], 0.80f, true);
        float ix = (scrW - iw) * 0.5f;

        if (sel) {
            float pulse = 0.10f + 0.05f * sinf(blinkTimer * 5.0f);
            drawRectGradient(px + 10, yPos - 10, pw - 20, 48.0f,
                pulse, 0.0f, 0.0f, 0.85f,
                0.0f, 0.0f, 0.0f, 0.0f);
            drawRect(px + 10, yPos - 10, 3.0f, 48.0f,
                0.9f, 0.0f, 0.0f, 1.0f);
            float rb = 0.88f + 0.12f * sinf(blinkTimer * 6.0f);
            drawText(items[i], ix, yPos + 34, 0.80f,
                rb, 0.15f, 0.15f, 1.0f, true);
        }
        else {
            drawText(items[i], ix, yPos + 34, 0.80f,
                0.40f, 0.40f, 0.40f, 1.0f, true);
        }
    }

    std::string hint = "ESC to resume";
    float hintW = measureText(hint, 0.48f, false);
    drawText(hint, (scrW - hintW) * 0.5f, py + ph - 20.0f,
        0.48f, 0.28f, 0.0f, 0.0f, 0.80f, false);
}

// ============================================================
//  renderWin
// ============================================================
void Menu::renderWin() {
    // Fondo negro solido
    drawRect(0, 0, (float)scrW, (float)scrH, 0.0f, 0.0f, 0.0f, 1.0f);

    float blink = 0.75f + 0.25f * sinf(blinkTimer * 2.2f);

    // Titulo principal
    float tw = measureText("GANASTE", 1.0f, true);
    float tx = (scrW - tw) * 0.5f;
    float ty = scrH * 0.42f;

    // Sombra
    drawText("GANASTE", tx + 3, ty + 3, 1.0f,
        0.0f, 0.0f, 0.0f, 0.6f, true);
    // Titulo con pulso suave
    drawText("GANASTE", tx, ty, 1.0f,
        blink, blink, blink, 1.0f, true);

    // Linea decorativa
    float lineY = ty + 24.0f;
    float lineW = scrW * 0.25f;
    float lineX = scrW * 0.25f;
    drawRectGradient(lineX, lineY, lineW, 2.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.8f, 0.8f, 0.8f, 1.0f);
    drawRectGradient(lineX + lineW, lineY, lineW, 2.0f,
        0.8f, 0.8f, 0.8f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    // Hint para continuar
    std::string hint = "Press ENTER or click to return to main menu";
    float hw = measureText(hint, 0.55f, false);
    float hintAlpha = 0.5f + 0.4f * sinf(blinkTimer * 2.0f);
    drawText(hint, (scrW - hw) * 0.5f, scrH * 0.62f,
        0.55f, 0.7f, 0.7f, 0.7f, hintAlpha, false);
}