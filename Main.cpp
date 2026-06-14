#include "Main.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <glm/gtc/type_ptr.hpp>

// ---- Vertex shader 2D ----
static const char* quadVS = R"(
#version 330 core
layout(location = 0) in vec4 vertex; // xy=pos, zw=uv
out vec2 TexCoords;
void main() {
    gl_Position = vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

// ---- Fragment shader 2D ----
static const char* quadFS = R"(
#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform vec4 color;
uniform bool useTexture;
uniform sampler2D fontTex;
void main() {
    if (useTexture) {
        float alpha = texture(fontTex, TexCoords).r;
        FragColor = vec4(color.rgb, color.a * alpha);
    } else {
        FragColor = color;
    }
}
)";

static unsigned int compileShader(const char* vs, const char* fs) {
    int ok; char log[512];
    unsigned int v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vs, NULL); glCompileShader(v);
    glGetShaderiv(v, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(v, 512, NULL, log); std::cout << "Menu VS: " << log << "\n"; }

    unsigned int f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fs, NULL); glCompileShader(f);
    glGetShaderiv(f, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(f, 512, NULL, log); std::cout << "Menu FS: " << log << "\n"; }

    unsigned int p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

Menu::Menu(int screenWidth, int screenHeight)
    : scrW(screenWidth), scrH(screenHeight)
{
    compileQuadShader();
    setupQuadBuffers();
    fontLoaded = loadFont("Resources/fonts/RubikDistressed-Regular.ttf");
    if (!fontLoaded)
        std::cout << "Menu: No se pudo cargar la fuente\n";
}

Menu::~Menu() {
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteProgram(quadShader);
    if (fontTexture) glDeleteTextures(1, &fontTexture);
}

void Menu::compileQuadShader() {
    quadShader = compileShader(quadVS, quadFS);
}

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

bool Menu::loadFont(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<unsigned char> buf((std::istreambuf_iterator<char>(f)), {});

    const int BMAP_W = 512, BMAP_H = 512;
    std::vector<unsigned char> bitmap(BMAP_W * BMAP_H);
    stbtt_BakeFontBitmap(buf.data(), 0, 48.0f, bitmap.data(), BMAP_W, BMAP_H, 32, 96, cdata);

    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, BMAP_W, BMAP_H, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return true;
}

void Menu::SetLoadingProgress(float p) {
    loadingProgress = p;
}

glm::vec2 Menu::toNDC(float x, float y) {
    return glm::vec2(
        (x / scrW) * 2.0f - 1.0f,
        1.0f - (y / scrH) * 2.0f
    );
}

void Menu::drawRect(float x, float y, float w, float h,
    float r, float g, float b, float a)
{
    glm::vec2 tl = toNDC(x, y);
    glm::vec2 br = toNDC(x + w, y + h);

    float verts[6][4] = {
        {tl.x, tl.y, 0,0}, {br.x, tl.y, 0,0}, {br.x, br.y, 0,0},
        {tl.x, tl.y, 0,0}, {br.x, br.y, 0,0}, {tl.x, br.y, 0,0}
    };

    glUseProgram(quadShader);
    glUniform4f(glGetUniformLocation(quadShader, "color"), r, g, b, a);
    glUniform1i(glGetUniformLocation(quadShader, "useTexture"), 0);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Menu::drawText(const std::string& text, float x, float y,
    float scale, float r, float g, float b, float a)
{
    if (!fontLoaded || text.empty()) return;

    glUseProgram(quadShader);
    glUniform4f(glGetUniformLocation(quadShader, "color"), r, g, b, a);
    glUniform1i(glGetUniformLocation(quadShader, "useTexture"), 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glUniform1i(glGetUniformLocation(quadShader, "fontTex"), 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float cx = x;
    for (char c : text) {
        if (c < 32 || c > 126) continue;
        stbtt_aligned_quad q;
        float dummy = 0;
        stbtt_GetBakedQuad(cdata, 512, 512, c - 32, &cx, &dummy, &q, 1);

        float qx0 = x + (q.x0 - x);
        float qx1 = x + (q.x1 - x);
        float qy0 = y + q.y0 * scale;
        float qy1 = y + q.y1 * scale;

        glm::vec2 tl = toNDC(qx0, qy0);
        glm::vec2 br = toNDC(qx1, qy1);

        float verts[6][4] = {
            {tl.x, tl.y, q.s0, q.t0}, {br.x, tl.y, q.s1, q.t0}, {br.x, br.y, q.s1, q.t1},
            {tl.x, tl.y, q.s0, q.t0}, {br.x, br.y, q.s1, q.t1}, {tl.x, br.y, q.s0, q.t1}
        };

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
}

bool Menu::Update(GLFWwindow* window, float deltaTime) {
    blinkTimer += deltaTime;
    hoverTimer += deltaTime;

    bool clickNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (state == MenuState::MAIN) {
        // ==========================================================
        // MOUSE CORREGIDO PARA PANTALLA COMPLETA / VENTANA
        // ==========================================================
        double rawMouseX, rawMouseY;
        glfwGetCursorPos(window, &rawMouseX, &rawMouseY);

        int realWidth, realHeight;
        glfwGetWindowSize(window, &realWidth, &realHeight);

        if (realWidth <= 0) realWidth = scrW;
        if (realHeight <= 0) realHeight = scrH;

        float scaleX = (float)scrW / (float)realWidth;
        float scaleY = (float)scrH / (float)realHeight;

        mouseX = rawMouseX * scaleX;
        mouseY = rawMouseY * scaleY;

        hoveredItem = -1;

        for (int i = 0; i < 4; i++) {
            float yPos = scrH * 0.40f + i * 70.0f;
            float x0 = scrW * 0.35f;
            float y0 = yPos - 8;
            float x1 = x0 + scrW * 0.30f;
            float y1 = y0 + 48.0f;

            if (mouseX >= x0 && mouseX <= x1 && mouseY >= y0 && mouseY <= y1)
                hoveredItem = i;
        }

        if (hoveredItem != -1)
            selectedItem = hoveredItem;

        if (clickNow && !prevEnter && hoveredItem != -1) {
            prevEnter = true;

            if (selectedItem == 0) {
                state = MenuState::LOADING;
                loadingProgress = 0.0f;
            }
            else if (selectedItem == 1) {
                state = MenuState::SETTINGS;
            }
            else if (selectedItem == 2) {
                state = MenuState::CREDITS;
            }
            else if (selectedItem == 3) {
                glfwSetWindowShouldClose(window, true);
            }

            return false;
        }
    }
    else if (state == MenuState::SETTINGS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            mouseSensitivity = glm::clamp(mouseSensitivity - 0.1f * deltaTime, 0.01f, 0.30f);

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            mouseSensitivity = glm::clamp(mouseSensitivity + 0.1f * deltaTime, 0.01f, 0.30f);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            state = MenuState::MAIN;
        }
    }
    else if (state == MenuState::CREDITS) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            state = MenuState::MAIN;
    }
    else if (state == MenuState::LOADING) {
        if (loadingProgress >= 1.0f) {
            state = MenuState::PLAYING;
            return true;
        }
    }

    prevEnter = clickNow;
    return false;
}

void Menu::Render() {
    // Configuración base de OpenGL 2D
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // PASO 1: Pintar el fondo negro común una sola vez al principio
    drawRect(0, 0, (float)scrW, (float)scrH, 0.0f, 0.0f, 0.0f, 1.0f);

    GLFWwindow* activeWindow = glfwGetCurrentContext();

    // PASO 2: Dibujar únicamente las capas superiores de cada estado
    if (state == MenuState::MAIN) {
        renderMain(0.016f);
    }
    else if (state == MenuState::SETTINGS) {
        renderSettings(activeWindow);
    }
    else if (state == MenuState::CREDITS) {
        renderCredits();
    }
    else if (state == MenuState::LOADING) {
        renderLoading();
    }

    glEnable(GL_DEPTH_TEST);
}

void Menu::renderMain(float dt) {
    // Ya no limpiamos el fondo aquí para evitar sobreescrituras corruptas
    float blink = 0.7f + 0.3f * sinf(blinkTimer * 3.0f);
    float titleX = scrW * 0.5f - (6.0f * 26.0f * 1.0f) * 0.5f;
    drawText("HUNTED", titleX, scrH * 0.18f, 1.0f, blink, 0.0f, 0.0f, 1.0f);

    drawRect(scrW * 0.3f, scrH * 0.28f, scrW * 0.4f, 2.0f, 0.6f, 0.0f, 0.0f, 1.0f);

    const char* items[] = { "PLAY", "SETTINGS", "CREDITS", "QUIT" };
    int lengths[] = { 4, 8, 7, 4 };

    for (int i = 0; i < 4; i++) {
        float yPos = scrH * 0.40f + i * 70.0f;
        bool selected = (i == selectedItem) || (i == hoveredItem);
        float itemX = scrW * 0.5f - (lengths[i] * 25.0f * 1.0f) * 0.5f;

        if (selected) {
            float pulse = 0.15f + 0.05f * sinf(blinkTimer * 4.0f);
            drawRect(scrW * 0.35f, yPos - 8, scrW * 0.30f, 48.0f, pulse, 0.0f, 0.0f, 0.8f);

            float rb = 0.85f + 0.15f * sinf(blinkTimer * 5.0f);
            drawText(items[i], itemX, yPos + 36, 1.0f, rb, 0.1f, 0.1f, 1.0f);
        }
        else {
            drawText(items[i], itemX, yPos + 36, 1.0f, 0.45f, 0.45f, 0.45f, 1.0f);
        }
    }
}

void Menu::renderSettings(GLFWwindow* window) {
    // Renderizado limpio de los elementos sobre el fondo oscuro ya dibujado
    float titleX = scrW * 0.5f - (8.0f * 26.0f * 1.0f) * 0.5f;
    drawText("SETTINGS", titleX, scrH * 0.15f, 1.0f, 0.8f, 0.0f, 0.0f, 1.0f);

    drawRect(scrW * 0.3f, scrH * 0.25f, scrW * 0.4f, 2.0f, 0.4f, 0.0f, 0.0f, 1.0f);

    float sensX = scrW * 0.5f - (17.0f * 24.0f * 0.75f) * 0.5f;
    drawText("MOUSE SENSITIVITY", sensX, scrH * 0.40f, 0.75f, 0.7f, 0.7f, 0.7f, 1.0f);

    float barX = scrW * 0.25f;
    float barY = scrH * 0.47f;
    float barW = scrW * 0.50f;
    float barH = 18.0f;

    drawRect(barX, barY, barW, barH, 0.2f, 0.0f, 0.0f, 1.0f);

    float factor = (mouseSensitivity - 0.01f) / (0.30f - 0.01f);
    factor = glm::clamp(factor, 0.0f, 1.0f);

    float fillW = barW * factor;
    if (fillW > 0.0f) {
        drawRect(barX, barY, fillW, barH, 0.8f, 0.0f, 0.0f, 1.0f);
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%.3f", mouseSensitivity);
    float valX = scrW * 0.5f - (5.0f * 24.0f * 0.75f) * 0.5f;
    drawText(buf, valX, scrH * 0.56f, 0.75f, 1.0f, 0.3f, 0.3f, 1.0f);

    float footX = scrW * 0.5f - (41.0f * 23.0f * 0.55f) * 0.5f;
    drawText("LEFT / RIGHT to adjust  |  ESC to go back", footX, scrH * 0.92f, 0.55f, 0.4f, 0.0f, 0.0f, 1.0f);
}

void Menu::renderCredits() {
    float titleX = scrW * 0.5f - (7.0f * 26.0f * 1.0f) * 0.5f;
    drawText("CREDITS", titleX, scrH * 0.15f, 1.0f, 0.8f, 0.0f, 0.0f, 1.0f);

    drawRect(scrW * 0.3f, scrH * 0.25f, scrW * 0.4f, 2.0f, 0.4f, 0.0f, 0.0f, 1.0f);

    float contentX = scrW * 0.5f - (13.0f * 24.0f * 0.75f) * 0.5f;
    drawText("Coming soon...", contentX, scrH * 0.50f, 0.75f, 0.5f, 0.5f, 0.5f, 1.0f);

    float footX = scrW * 0.5f - (14.0f * 23.0f * 0.55f) * 0.5f;
    drawText("ESC to go back", footX, scrH * 0.92f, 0.55f, 0.4f, 0.0f, 0.0f, 1.0f);
}

void Menu::renderLoading() {
    float blink = 0.5f + 0.5f * sinf(blinkTimer * 4.0f);
    float loadX = scrW * 0.5f - (10.0f * 26.0f * 1.0f) * 0.5f;
    drawText("LOADING...", loadX, scrH * 0.38f, 1.0f, blink, 0.0f, 0.0f, 1.0f);

    float barX = scrW * 0.15f;
    float barY = scrH * 0.55f;
    float barW = scrW * 0.70f;
    float barH = 28.0f;

    drawRect(barX - 3, barY - 3, barW + 6, barH + 6, 0.5f, 0.0f, 0.0f, 1.0f);
    drawRect(barX, barY, barW, barH, 0.04f, 0.0f, 0.0f, 1.0f);

    float fillW = barW * loadingProgress;
    if (fillW > 2.0f) {
        drawRect(barX, barY, fillW, barH, 0.22f, 0.0f, 0.0f, 1.0f);
        drawRect(barX, barY + barH * 0.3f, fillW, barH * 0.7f, 0.35f, 0.0f, 0.0f, 1.0f);
        float glowW = glm::min(fillW, 20.0f);
        drawRect(barX + fillW - glowW, barY, glowW, barH, 0.95f, 0.05f, 0.05f, 1.0f);
        drawRect(barX, barY, fillW, barH * 0.2f, 0.7f, 0.0f, 0.0f, 0.35f);

        for (int d = 0; d < 6; d++) {
            float speed = 50.0f + d * 20.0f;
            float period = 80.0f + d * 25.0f;
            float offset = fmodf(blinkTimer * speed + d * 53.0f, period);
            float dropX = barX + fillW - 4.0f - d * 5.0f;
            float dropY = barY + barH + offset;
            float alpha = glm::clamp(1.0f - offset / period, 0.0f, 1.0f);
            float bright = 0.55f + 0.4f * (1.0f - offset / period);

            float dW = 7.0f - d * 0.5f;
            float dH = 10.0f + d * 4.0f;
            drawRect(dropX, dropY, dW, dH * 0.6f, bright, 0.0f, 0.0f, alpha);
            drawRect(dropX + dW * 0.2f, dropY + dH * 0.6f, dW * 0.6f, dH * 0.4f, bright * 0.7f, 0.0f, 0.0f, alpha * 0.7f);
            drawRect(dropX + dW * 0.35f, dropY + dH, dW * 0.3f, dH * 0.2f, bright * 0.5f, 0.0f, 0.0f, alpha * 0.4f);

            if (offset > period * 0.85f) {
                float spreadW = (offset - period * 0.85f) / (period * 0.15f) * 20.0f;
                drawRect(dropX - spreadW * 0.5f, dropY + dH, dW + spreadW, 3.0f, 0.4f, 0.0f, 0.0f, alpha * 0.6f);
            }
        }
    }

    char buf[16];
    int pct = (int)(loadingProgress * 100);
    snprintf(buf, sizeof(buf), "%d%%", pct);
    int chars = (pct >= 100) ? 4 : ((pct >= 10) ? 3 : 2);
    float pctX = scrW * 0.5f - (chars * 24.0f * 0.75f) * 0.5f;
    drawText(buf, pctX, scrH * 0.72f, 0.75f, 0.8f, 0.0f, 0.0f, 1.0f);
}