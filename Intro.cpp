#include "Intro.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <cmath>
#include <cstdlib>

static const char* INTRO_VS = R"(
#version 330 core
layout(location = 0) in vec4 vertex;
out vec2 TexCoords;
void main() {
    gl_Position = vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

static const char* INTRO_FS = R"(
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

Intro::Intro(int screenWidth, int screenHeight)
    : scrW(screenWidth), scrH(screenHeight)
{
    slides.push_back({ "Laboratorio Universitario — Sotano B2\n23:47 horas", 4.0f });
    slides.push_back({ "Era una noche como cualquier otra para Alex.\nSolo en el laboratorio del sotano,\nterminando su proyecto final.", 5.0f });
    slides.push_back({ "Su companero aparecio con un cafe.\n\"Te lo mereces despues de tanto trabajo\", dijo.\nAlex bebio sin pensarlo dos veces.", 5.0f });
    slides.push_back({ "Entonces... todo se apago.", 3.5f });
    slides.push_back({ "Al despertar, el laboratorio era irreconocible.\nDestruido. Oscuro. En silencio absoluto.\nComo si el tiempo hubiera retrocedido.", 5.5f });
    slides.push_back({ "Algo se mueve en la oscuridad.\nAlgo que no deberia estar aqui.\nAlgo que ya sabe que estas despierto.", 5.0f });
    slides.push_back({ "Solo tienes una linterna.\nEncuentra las llaves.\nSal antes de que te encuentre.", 5.0f });
    slides.push_back({ "No estas solo.", 3.5f });
}

Intro::~Intro()
{
    if (quadVAO)     glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO)     glDeleteBuffers(1, &quadVBO);
    if (quadShader)  glDeleteProgram(quadShader);
    if (fontTexture) glDeleteTextures(1, &fontTexture);
}

bool Intro::Init(const std::string& fontPath)
{
    compileShader();
    setupBuffers();
    fontLoaded = loadFont(fontPath);
    return fontLoaded;
}

void Intro::Update(float deltaTime, AudioManager* audio)
{
    if (finished) return;
    if (currentSlide >= (int)slides.size()) { finished = true; return; }


    const IntroSlide& slide = slides[currentSlide];

    // --- Parpadeo del cursor ---
    cursorBlinkTimer += deltaTime;
    if (cursorBlinkTimer >= 0.5f)
    {
        cursorBlinkTimer = 0.0f;
        cursorVisible = !cursorVisible;
    }

    // --- Niebla al terminar de escribir ---
    if (fogActive)
    {
        fogTimer += deltaTime;
        fogAlpha = glm::clamp(fogTimer / 1.5f, 0.0f, 1.0f);
        if (fogTimer > 3.0f)
        {
            fogActive = false;
            fogAlpha = 0.0f;
        }
    }

    // --- Estatica entre slides ---
    staticTimer += deltaTime;
    if (showStatic)
    {
        staticAlpha = glm::clamp(staticAlpha + deltaTime * 3.0f, 0.0f, 0.5f);
        if (staticTimer > 0.4f)
        {
            showStatic = false;
            staticAlpha = 0.0f;
            staticTimer = 0.0f;
        }
    }

    // --- Maquina de escribir ---
   // --- Mostrar el parrafo completo de una vez (sin maquina de escribir) ---
    if (!typingDone)
    {
        visibleChars = (int)slide.text.size(); // todo el texto visible ya
        typingDone = true;
        fadingIn = true;
        fadeAlpha = 0.0f;   // arranca transparente y se desvanece hacia visible
        fogActive = true;
        fogTimer = 0.0f;
        fogAlpha = 0.0f;
        return;
    }

    // --- Fade in ---
    if (fadingIn)
    {
        fadeAlpha += deltaTime / fadeInTime;
        if (fadeAlpha >= 1.0f) { fadeAlpha = 1.0f; fadingIn = false; }
        return;
    }

    // --- Esperar tiempo del slide ---
    slideTimer += deltaTime;
    if (slideTimer < slide.displayTime) return;

    // --- Fade out ---
    if (!fadingOut)
    {
        fadingOut = true;
        fadeAlpha = 1.0f;
    }

    fadeAlpha -= deltaTime / fadeOutTime;

    if (fadeAlpha <= 0.0f)
    {
        fadeAlpha = 0.0f;

        // =====================================================
        // AUDIO — estatica entre slides
        // Descomenta cuando tengas el archivo:
        // audio->Play("estatica");
        // =====================================================

        showStatic = true;
        staticTimer = 0.0f;

        // Resetear todo para el siguiente slide
        currentSlide++;
        slideTimer = 0.0f;
        fadingIn = true;
        fadingOut = false;
        fadeAlpha = 0.0f;
        typeTimer = 0.0f;
        visibleChars = 0;
        typingDone = false;
        fogActive = false;
        fogAlpha = 0.0f;
        fogTimer = 0.0f;
        cursorBlinkTimer = 0.0f;
        cursorVisible = true;

        if (currentSlide >= (int)slides.size())
            finished = true;
    }
}

void Intro::Render()
{
    if (finished) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Fondo negro
    drawRect(0, 0, (float)scrW, (float)scrH, 0.0f, 0.0f, 0.0f, 1.0f);

    if (currentSlide >= (int)slides.size()) return;

    const IntroSlide& slide = slides[currentSlide];
    std::string visible = slide.text.substr(0, visibleChars);

    // Separar por lineas
    std::vector<std::string> lines;
    std::string current;
    for (char c : visible)
    {
        if (c == '\n') { lines.push_back(current); current.clear(); }
        else current += c;
    }
    if (!current.empty()) lines.push_back(current);

    float lineH = 55.0f;
    float totalH = lines.size() * lineH;
    float startY = scrH * 0.5f - totalH * 0.5f;
    float alpha = fadeAlpha;

    // Dibujar texto
    for (int i = 0; i < (int)lines.size(); i++)
    {
        float tw = textWidth(lines[i], 0.7f);
        float tx = scrW * 0.5f - tw * 0.5f;
        float ty = startY + i * lineH;
        drawText(lines[i], tx, ty, 0.7f, 0.85f, 0.85f, 0.85f, alpha);
    }

    // --- Cursor parpadeante (barra gruesa) ---
    if (!typingDone && cursorVisible && !lines.empty())
    {
        float lastLineW = textWidth(lines.back(), 0.7f);
        float lastLineX = scrW * 0.5f - lastLineW * 0.5f;
        float cursorX = lastLineX + lastLineW + 6.0f;

        float cursorWidth = 6.0f;
        float cursorHeight = 34.0f;
        float cursorY = startY + (lines.size() - 1) * lineH - 24.0f;

        drawRect(cursorX, cursorY, cursorWidth, cursorHeight, 0.9f, 0.9f, 0.9f, alpha);
    }

    // --- Efecto niebla al terminar ---
    if (fogActive && fogAlpha > 0.0f)
    {
        drawRect(0, 0, (float)scrW, (float)scrH, 0.0f, 0.0f, 0.0f, fogAlpha * 0.5f);
        drawRect(0, 0, (float)scrW, (float)scrH, 0.0f, 0.0f, 0.0f, fogAlpha * 0.35f);
        drawRect(0, 0, (float)scrW, (float)scrH, 0.0f, 0.0f, 0.0f, fogAlpha * 0.2f);

        if (fogAlpha > 0.3f)
            drawStatic(fogAlpha * 0.35f);
    }

    // --- Estatica entre slides ---
    if (showStatic)
        drawStatic(staticAlpha);

    glEnable(GL_DEPTH_TEST);
}

// ==================================================
// Helpers
// ==================================================
void Intro::drawStatic(float alpha)
{
    for (int i = 0; i < 80; i++)
    {
        float rx = (float)(rand() % scrW);
        float ry = (float)(rand() % scrH);
        float rw = 2.0f + rand() % 6;
        float rh = 1.0f + rand() % 3;
        float br = 0.3f + ((float)rand() / RAND_MAX) * 0.7f;
        drawRect(rx, ry, rw, rh, br, br, br, alpha);
    }
}

float Intro::textWidth(const std::string& text, float scale)
{
    if (!fontLoaded) return 0.0f;
    float cx = 0.0f, cy = 0.0f;
    for (char c : text)
    {
        if (c < 32 || c > 126) continue;
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(cdata, 512, 512, c - 32, &cx, &cy, &q, 1);
    }
    return cx * scale;
}

glm::vec2 Intro::toNDC(float x, float y)
{
    return glm::vec2(
        (x / scrW) * 2.0f - 1.0f,
        1.0f - (y / scrH) * 2.0f
    );
}

void Intro::drawRect(float x, float y, float w, float h,
    float r, float g, float b, float a)
{
    glm::vec2 tl = toNDC(x, y);
    glm::vec2 br = toNDC(x + w, y + h);

    float verts[6][4] = {
        {tl.x,tl.y,0,0},{br.x,tl.y,0,0},{br.x,br.y,0,0},
        {tl.x,tl.y,0,0},{br.x,br.y,0,0},{tl.x,br.y,0,0}
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

void Intro::drawText(const std::string& text, float x, float y,
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
    for (char c : text)
    {
        if (c < 32 || c > 126) continue;
        stbtt_aligned_quad q;
        float dummy = 0;
        stbtt_GetBakedQuad(cdata, 512, 512, c - 32, &cx, &dummy, &q, 1);

        float qy0 = y + q.y0 * scale;
        float qy1 = y + q.y1 * scale;

        glm::vec2 tl = toNDC(q.x0, qy0);
        glm::vec2 br = toNDC(q.x1, qy1);

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

void Intro::compileShader()
{
    auto compile = [](GLenum type, const char* src) -> unsigned int
        {
            unsigned int id = glCreateShader(type);
            glShaderSource(id, 1, &src, nullptr);
            glCompileShader(id);
            int ok; char log[512];
            glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
            if (!ok) { glGetShaderInfoLog(id, 512, nullptr, log); std::cout << "[Intro] Shader: " << log << "\n"; }
            return id;
        };

    unsigned int vs = compile(GL_VERTEX_SHADER, INTRO_VS);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, INTRO_FS);
    quadShader = glCreateProgram();
    glAttachShader(quadShader, vs);
    glAttachShader(quadShader, fs);
    glLinkProgram(quadShader);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void Intro::setupBuffers()
{
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

bool Intro::loadFont(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) { std::cout << "[Intro] No se encontro la fuente: " << path << "\n"; return false; }
    std::vector<unsigned char> buf((std::istreambuf_iterator<char>(f)), {});

    const int BW = 512, BH = 512;
    std::vector<unsigned char> bitmap(BW * BH);
    stbtt_BakeFontBitmap(buf.data(), 0, 42.0f, bitmap.data(), BW, BH, 32, 96, cdata);

    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, BW, BH, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return true;
}