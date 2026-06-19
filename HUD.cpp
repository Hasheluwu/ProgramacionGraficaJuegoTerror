#include "HUD.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_truetype.h"

#include <fstream>
#include <vector>
#include <iostream>

static const char* TEXT_VERT = R"(
#version 330 core
layout (location = 0) in vec4 vertex; // xy = pos, zw = uv
out vec2 TexCoord;
uniform mat4 projection;
void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoord    = vertex.zw;
}
)";

static const char* TEXT_FRAG = R"(
#version 330 core
in  vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D fontAtlas;
uniform vec4      textColor;
void main()
{
    float alpha = texture(fontAtlas, TexCoord).r;
    FragColor   = vec4(textColor.rgb, textColor.a * alpha);
}
)";

static const char* RECT_VERT = R"(
#version 330 core
layout (location = 0) in vec2 pos;
uniform mat4 projection;
uniform vec2 rectPos;
uniform vec2 rectSize;
void main()
{
    vec2 finalPos = rectPos + pos * rectSize;
    gl_Position = projection * vec4(finalPos, 0.0, 1.0);
}
)";

static const char* RECT_FRAG = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 rectColor;
void main()
{
    FragColor = rectColor;
}
)";

HUD::HUD() {}
HUD::~HUD()
{
    if (VAO)              glDeleteVertexArrays(1, &VAO);
    if (VBO)              glDeleteBuffers(1, &VBO);
    if (fontTexture)      glDeleteTextures(1, &fontTexture);
    if (shaderProgram)    glDeleteProgram(shaderProgram);
    if (rectVAO)          glDeleteVertexArrays(1, &rectVAO);
    if (rectVBO)          glDeleteBuffers(1, &rectVBO);
    if (rectShaderProgram) glDeleteProgram(rectShaderProgram);
}

bool HUD::Init(const std::string& fontPath, int w, int h)
{
    screenW = w;
    screenH = h;

    if (!BuildShader()) return false;
    if (!BuildFont(fontPath)) return false;
    if (!BuildRectShader()) return false;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    UpdateProjection();

    ready = true;
    std::cout << "[HUD] Inicializado correctamente." << std::endl;
    return true;
}

void HUD::Resize(int w, int h)
{
    screenW = w;
    screenH = h;
    UpdateProjection();
}

void HUD::Render(const HUDState& s)
{
    GLboolean depthTest, blend;
    glGetBooleanv(GL_DEPTH_TEST, &depthTest);
    glGetBooleanv(GL_BLEND, &blend);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float cx = screenW * 0.5f;
    float cy = screenH * 0.5f;
    float barW = 200.0f;
    float barH = 18.0f;
    float barX = (float)screenW - barW - 20.0f;
    float barY = (float)screenH - 40.0f;

    float pct = glm::clamp(s.stamina / s.staminaMax, 0.0f, 1.0f);

    // Mira (crosshair)
    DrawText("+", cx - 6.0f, cy - 10.0f, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 0.75f));

    // Barra de stamina
    DrawRect(barX, barY, barW, 2.0f, glm::vec4(1, 1, 1, 0.25f));
    DrawRect(barX, barY + barH - 2.0f, barW, 2.0f, glm::vec4(1, 1, 1, 0.25f));
    DrawRect(barX, barY, 2.0f, barH, glm::vec4(1, 1, 1, 0.25f));
    DrawRect(barX + barW - 2.0f, barY, 2.0f, barH, glm::vec4(1, 1, 1, 0.25f));
    DrawRect(barX - 2.0f, barY - 2.0f, barW + 4.0f, barH + 4.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));

    glm::vec4 fillColor;
    if (s.isExhausted)
        fillColor = glm::vec4(0.6f, 0.15f, 0.15f, 0.9f);
    else if (pct < 0.3f)
        fillColor = glm::vec4(0.85f, 0.65f, 0.1f, 0.9f);
    else
        fillColor = glm::vec4(0.2f, 0.75f, 0.3f, 0.9f);

    DrawRect(barX, barY, barW * pct, barH, fillColor);

    if (!ready) return;

    // Llave 1
    if (s.hasKey)
        DrawText("[LLAVE]", 20.0f, (float)screenH - 40.0f, 1.0f, glm::vec4(1.0f, 0.85f, 0.1f, 1.0f));

    // Llave 2
    if (s.hasKey2)
        DrawText("[LLAVE 2]", 20.0f, (float)screenH - 70.0f, 1.0f, glm::vec4(0.3f, 0.7f, 1.0f, 1.0f));

    // Llave 3
    if (s.hasKey3)
        DrawText("[LLAVE 3]", 20.0f, (float)screenH - 100.0f, 1.0f, glm::vec4(0.8f, 0.2f, 0.8f, 1.0f));

    // Mensajes de llave recogida
    if (s.showKeyPickedMsg)
    {
        float alpha = glm::clamp(s.keyPickedMsgTimer, 0.0f, 1.0f);
        std::string msg = "!Llave recogida!";
        float tw = (float)msg.size() * 10.0f;
        DrawText(msg, cx - tw * 0.5f, cy - 90.0f, 1.0f, glm::vec4(1.0f, 0.85f, 0.1f, alpha));

        std::string sub = "Ahora puedes abrir las puertas bloqueadas";
        float sw = (float)sub.size() * 8.0f;
        DrawText(sub, cx - sw * 0.5f, cy - 64.0f, 0.85f, glm::vec4(1.0f, 1.0f, 1.0f, alpha * 0.8f));
    }

    if (s.showKey2PickedMsg)
    {
        float alpha = glm::clamp(s.key2PickedMsgTimer, 0.0f, 1.0f);
        std::string msg = "!Llave 2 recogida!";
        float tw = (float)msg.size() * 10.0f;
        DrawText(msg, cx - tw * 0.5f, cy - 150.0f, 1.0f, glm::vec4(0.3f, 0.7f, 1.0f, alpha));

        std::string sub = "Puede abrir otra puerta bloqueada";
        float sw = (float)sub.size() * 8.0f;
        DrawText(sub, cx - sw * 0.5f, cy - 124.0f, 0.85f, glm::vec4(1.0f, 1.0f, 1.0f, alpha * 0.8f));
    }

    if (s.showKey3PickedMsg)
    {
        float alpha = glm::clamp(s.key3PickedMsgTimer, 0.0f, 1.0f);
        std::string msg = "!Llave 3 recogida!";
        float tw = (float)msg.size() * 10.0f;
        DrawText(msg, cx - tw * 0.5f, cy - 210.0f, 1.0f, glm::vec4(0.8f, 0.2f, 0.8f, alpha));

        std::string sub = "Puede abrir otra puerta bloqueada";
        float sw = (float)sub.size() * 8.0f;
        DrawText(sub, cx - sw * 0.5f, cy - 184.0f, 0.85f, glm::vec4(1.0f, 1.0f, 1.0f, alpha * 0.8f));
    }

    // Item recogible
    if (s.lookingAtItem)
    {
        std::string msg = "[E] Recoger";
        float tw = (float)msg.size() * 10.0f;
        DrawText(msg, cx - tw * 0.5f, cy + 40.0f, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // ---- Puerta del puzzle (nuevo) ----
    if (s.lookingAtPuzzleDoor)
    {
        if (s.puzzleDoorBlocked)
        {
            std::string msg = "Resuelve el puzzle para abrir";
            float tw = (float)msg.size() * 8.0f;
            DrawText(msg, cx - tw * 0.5f, cy + 40.0f, 0.9f, glm::vec4(1.0f, 0.6f, 0.1f, 1.0f));
        }
        else
        {
            if (!s.doorIsOpen)
            {
                std::string msg = "[E] Abrir puerta";
                float tw = (float)msg.size() * 10.0f;
                DrawText(msg, cx - tw * 0.5f, cy + 40.0f, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            else
            {
                std::string msg = "[E] Cerrar puerta";
                float tw = (float)msg.size() * 10.0f;
                DrawText(msg, cx - tw * 0.5f, cy + 40.0f, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
        }
    }
    // ---- Puertas normales ----
    else if (s.lookingAtDoor && !s.doorIsOpen)
    {
        bool bloqueada1 = s.doorRequiresKey && !s.hasKey;
        bool bloqueada2 = s.doorRequiresKey2 && !s.hasKey2;
        bool bloqueada3 = s.doorRequiresKey3 && !s.hasKey3;
        bool bloqueada = bloqueada1 || bloqueada2 || bloqueada3;

        if (bloqueada)
        {
            std::string msg;
            glm::vec4   color;
            if (bloqueada3) { msg = "Necesitas la llave 3"; color = glm::vec4(0.8f, 0.2f, 0.8f, 1.0f); }
            else if (bloqueada2) { msg = "Necesitas la llave 2"; color = glm::vec4(0.3f, 0.7f, 1.0f, 1.0f); }
            else { msg = "Necesitas la llave 1"; color = glm::vec4(1.0f, 0.25f, 0.25f, 1.0f); }

            float tw = (float)msg.size() * 10.0f;
            DrawText(msg, cx - tw * 0.5f, cy + 40.0f, 1.0f, color);

            std::string sub = "[E] Bloqueada";
            float sw = (float)sub.size() * 9.0f;
            DrawText(sub, cx - sw * 0.5f, cy + 64.0f, 0.9f, glm::vec4(color.r, color.g, color.b, 0.7f));
        }
        else
        {
            std::string msg = "[E] Abrir puerta";
            float tw = (float)msg.size() * 10.0f;
            DrawText(msg, cx - tw * 0.5f, cy + 40.0f, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }
    else if (s.lookingAtDoor && s.doorIsOpen)
    {
        std::string msg = "[E] Cerrar puerta";
        float tw = (float)msg.size() * 10.0f;
        DrawText(msg, cx - tw * 0.5f, cy + 40.0f, 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // Restaurar estado OpenGL
    if (depthTest) glEnable(GL_DEPTH_TEST);
    if (!blend)    glDisable(GL_BLEND);
}

void HUD::DrawText(const std::string& text, float x, float y, float scale, glm::vec4 color)
{
    glUseProgram(shaderProgram);
    glUniform4f(glGetUniformLocation(shaderProgram, "textColor"), color.r, color.g, color.b, color.a);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "fontAtlas"), 0);

    glBindVertexArray(VAO);

    float curX = x;
    float curY = y;

    for (char c : text)
    {
        int idx = (int)c - FIRST_CHAR;
        if (idx < 0 || idx >= NUM_CHARS) { curX += 8.0f * scale; continue; }

        const CharInfo& ch = chars[idx];

        float w = (ch.x1 - ch.x0) * scale;
        float h = (ch.y1 - ch.y0) * scale;
        float xp = curX + ch.xoff * scale;
        float yp = curY + ch.yoff * scale;

        float verts[6][4] = {
            { xp,     yp + h, ch.x0 / BITMAP_W, ch.y1 / BITMAP_H },
            { xp,     yp,     ch.x0 / BITMAP_W, ch.y0 / BITMAP_H },
            { xp + w, yp,     ch.x1 / BITMAP_W, ch.y0 / BITMAP_H },

            { xp,     yp + h, ch.x0 / BITMAP_W, ch.y1 / BITMAP_H },
            { xp + w, yp,     ch.x1 / BITMAP_W, ch.y0 / BITMAP_H },
            { xp + w, yp + h, ch.x1 / BITMAP_W, ch.y1 / BITMAP_H },
        };

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        curX += ch.xadvance * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool HUD::BuildShader()
{
    auto compile = [](GLenum type, const char* src) -> unsigned int
        {
            unsigned int id = glCreateShader(type);
            glShaderSource(id, 1, &src, nullptr);
            glCompileShader(id);
            int ok; glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[512]; glGetShaderInfoLog(id, 512, nullptr, log);
                std::cout << "[HUD] Shader error: " << log << std::endl;
            }
            return id;
        };

    unsigned int vs = compile(GL_VERTEX_SHADER, TEXT_VERT);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, TEXT_FRAG);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    int ok; glGetProgramiv(shaderProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(shaderProgram, 512, nullptr, log);
        std::cout << "[HUD] Link error: " << log << std::endl;
        return false;
    }
    return true;
}

bool HUD::BuildFont(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cout << "[HUD] No se encontro la fuente: " << path << std::endl;
        return false;
    }

    std::streamsize sz = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer((size_t)sz);
    file.read((char*)buffer.data(), sz);

    stbtt_bakedchar bakedChars[NUM_CHARS];
    std::vector<unsigned char> bitmap(BITMAP_W * BITMAP_H);

    fontHeight = 22.0f;
    int result = stbtt_BakeFontBitmap(
        buffer.data(), 0,
        fontHeight,
        bitmap.data(), BITMAP_W, BITMAP_H,
        FIRST_CHAR, NUM_CHARS,
        bakedChars
    );

    if (result == 0) {
        std::cout << "[HUD] Error al bakear la fuente." << std::endl;
        return false;
    }

    for (int i = 0; i < NUM_CHARS; i++)
    {
        chars[i].x0 = (float)bakedChars[i].x0;
        chars[i].y0 = (float)bakedChars[i].y0;
        chars[i].x1 = (float)bakedChars[i].x1;
        chars[i].y1 = (float)bakedChars[i].y1;
        chars[i].xoff = bakedChars[i].xoff;
        chars[i].yoff = bakedChars[i].yoff;
        chars[i].xadvance = bakedChars[i].xadvance;
    }

    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, BITMAP_W, BITMAP_H, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "[HUD] Fuente cargada: " << path << std::endl;
    return true;
}

void HUD::UpdateProjection()
{
    if (!shaderProgram) return;

    glm::mat4 proj = glm::ortho(0.0f, (float)screenW, (float)screenH, 0.0f, -1.0f, 1.0f);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

    if (rectShaderProgram)
    {
        glUseProgram(rectShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(rectShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
    }
}

bool HUD::BuildRectShader()
{
    auto compile = [](GLenum type, const char* src) -> unsigned int
        {
            unsigned int id = glCreateShader(type);
            glShaderSource(id, 1, &src, nullptr);
            glCompileShader(id);
            int ok; glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[512]; glGetShaderInfoLog(id, 512, nullptr, log);
                std::cout << "[HUD] Rect shader error: " << log << std::endl;
            }
            return id;
        };

    unsigned int vs = compile(GL_VERTEX_SHADER, RECT_VERT);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, RECT_FRAG);

    rectShaderProgram = glCreateProgram();
    glAttachShader(rectShaderProgram, vs);
    glAttachShader(rectShaderProgram, fs);
    glLinkProgram(rectShaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    int ok; glGetProgramiv(rectShaderProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(rectShaderProgram, 512, nullptr, log);
        std::cout << "[HUD] Rect link error: " << log << std::endl;
        return false;
    }

    float quad[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}

void HUD::DrawRect(float x, float y, float w, float h, glm::vec4 color)
{
    glUseProgram(rectShaderProgram);
    glUniform2f(glGetUniformLocation(rectShaderProgram, "rectPos"), x, y);
    glUniform2f(glGetUniformLocation(rectShaderProgram, "rectSize"), w, h);
    glUniform4f(glGetUniformLocation(rectShaderProgram, "rectColor"), color.r, color.g, color.b, color.a);

    glBindVertexArray(rectVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}