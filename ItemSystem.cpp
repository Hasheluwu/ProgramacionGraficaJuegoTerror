#include "ItemSystem.h"

#include <cstdlib>
#include <cmath>

static const float KEY_NORMAL_SCALE = 2.8f;

// Como ya achicaste los pedazos en Blender, dejalo en 1.0f.
// Si se ven grandes, bajalo a 0.6f.
static const float KEY1_PIECE_SCALE = 1.0f;

static const float KEY1_PIECE_SEPARATION = 0.75f;

ItemSystem::ItemSystem()
    : hasKey(false),
    hasKey2(false),
    hasKey3(false),
    key1PiecesCollected(0)
{
    // IMPORTANTE:
    // Ya NO cargamos Resources/Models/llaves/llave1.obj.
    // La llave 1 ahora SOLO existe como:
    // llave1_pedazo1.obj, llave1_pedazo2.obj, llave1_pedazo3.obj

    try
    {
        key1Piece1Model = new Model("Resources/Models/llaves/llave1_pedazo1.obj");
        std::cout << "[ITEMS] Modelo llave1_pedazo1.obj cargado correctamente." << std::endl;
    }
    catch (...)
    {
        key1Piece1Model = nullptr;
        std::cout << "[ITEMS] ADVERTENCIA: No se pudo cargar llave1_pedazo1.obj." << std::endl;
    }

    try
    {
        key1Piece2Model = new Model("Resources/Models/llaves/llave1_pedazo2.obj");
        std::cout << "[ITEMS] Modelo llave1_pedazo2.obj cargado correctamente." << std::endl;
    }
    catch (...)
    {
        key1Piece2Model = nullptr;
        std::cout << "[ITEMS] ADVERTENCIA: No se pudo cargar llave1_pedazo2.obj." << std::endl;
    }

    try
    {
        key1Piece3Model = new Model("Resources/Models/llaves/llave1_pedazo3.obj");
        std::cout << "[ITEMS] Modelo llave1_pedazo3.obj cargado correctamente." << std::endl;
    }
    catch (...)
    {
        key1Piece3Model = nullptr;
        std::cout << "[ITEMS] ADVERTENCIA: No se pudo cargar llave1_pedazo3.obj." << std::endl;
    }

    try
    {
        key2Model = new Model("Resources/Models/llaves/llave2.obj");
        std::cout << "[ITEMS] Modelo llave2.obj cargado correctamente." << std::endl;
    }
    catch (...)
    {
        key2Model = nullptr;
        std::cout << "[ITEMS] ADVERTENCIA: No se pudo cargar llave2.obj." << std::endl;
    }

    try
    {
        key3Model = new Model("Resources/Models/llaves/llave3.obj");
        std::cout << "[ITEMS] Modelo llave3.obj cargado correctamente." << std::endl;
    }
    catch (...)
    {
        key3Model = nullptr;
        std::cout << "[ITEMS] ADVERTENCIA: No se pudo cargar llave3.obj." << std::endl;
    }
}

ItemSystem::~ItemSystem()
{
    delete key2Model;
    delete key3Model;

    delete key1Piece1Model;
    delete key1Piece2Model;
    delete key1Piece3Model;

    key2Model = nullptr;
    key3Model = nullptr;

    key1Piece1Model = nullptr;
    key1Piece2Model = nullptr;
    key1Piece3Model = nullptr;
}

bool ItemSystem::IsKeyPiece(ItemType type)
{
    return type == ItemType::KEY1_PIECE1 ||
        type == ItemType::KEY1_PIECE2 ||
        type == ItemType::KEY1_PIECE3;
}

Model* ItemSystem::GetBestModelForItem(ItemType type)
{
    switch (type)
    {
    case ItemType::KEY:
        // llave1.obj fue eliminada del codigo.
        return nullptr;

    case ItemType::KEY2:
        return key2Model;

    case ItemType::KEY3:
        return key3Model;

    case ItemType::KEY1_PIECE1:
        return key1Piece1Model;

    case ItemType::KEY1_PIECE2:
        // Visualmente usamos el pedazo 1 porque el pedazo 2 no se esta mostrando bien.
        // La logica sigue siendo KEY1_PIECE2.
        if (key1Piece1Model) return key1Piece1Model;
        return key1Piece2Model;

    case ItemType::KEY1_PIECE3:
        // Visualmente usamos el pedazo 1 porque el pedazo 3 no se esta mostrando bien.
        // La logica sigue siendo KEY1_PIECE3.
        if (key1Piece1Model) return key1Piece1Model;
        return key1Piece3Model;

    default:
        return nullptr;
    }
}

void ItemSystem::SpawnKeyRandom(const std::vector<glm::vec3>& spawnPoints)
{
    if (spawnPoints.empty())
    {
        std::cout << "[ITEMS] ERROR: Lista de spawn points vacia para llave 1." << std::endl;
        return;
    }

    int index = rand() % spawnPoints.size();
    glm::vec3 pos = spawnPoints[index];

    // Compatibilidad vieja. No dibuja modelo llave1.obj, solo fallback/marker.
    items.push_back(Item(ItemType::KEY, pos, 4.0f, "recoger", false));

    std::cout << "[ITEMS] Llave 1 completa vieja spawneada sin modelo en: "
        << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
}

void ItemSystem::SpawnKey1PiecesAt(glm::vec3 basePosition)
{
    glm::vec3 p1 = basePosition + glm::vec3(-KEY1_PIECE_SEPARATION, 0.0f, 0.0f);
    glm::vec3 p2 = basePosition + glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 p3 = basePosition + glm::vec3(KEY1_PIECE_SEPARATION, 0.0f, 0.0f);

    items.push_back(Item(ItemType::KEY1_PIECE1, p1, 4.0f, "recoger", true));
    items.push_back(Item(ItemType::KEY1_PIECE2, p2, 4.0f, "recoger", true));
    items.push_back(Item(ItemType::KEY1_PIECE3, p3, 4.0f, "recoger", true));

    std::cout << "[ITEMS] Pedazos de llave 1 spawneados cerca de: "
        << basePosition.x << ", " << basePosition.y << ", " << basePosition.z << std::endl;
}

void ItemSystem::SpawnKey1PiecesRandom(const std::vector<glm::vec3>& spawnPoints)
{
    if (spawnPoints.empty())
    {
        std::cout << "[ITEMS] ERROR: Lista de spawn points vacia para pedazos de llave 1." << std::endl;
        return;
    }

    int index = rand() % spawnPoints.size();
    glm::vec3 basePos = spawnPoints[index];

    SpawnKey1PiecesAt(basePos);
}

void ItemSystem::SpawnKey1PiecesFixedPositions(const std::vector<glm::vec3>& piecePositions)
{
    if (piecePositions.size() < 3)
    {
        std::cout << "[ITEMS] ERROR: Se necesitan 3 posiciones para los pedazos de llave 1." << std::endl;
        return;
    }

    items.push_back(Item(ItemType::KEY1_PIECE1, piecePositions[0], 4.0f, "recoger", true));
    items.push_back(Item(ItemType::KEY1_PIECE2, piecePositions[1], 4.0f, "recoger", true));
    items.push_back(Item(ItemType::KEY1_PIECE3, piecePositions[2], 4.0f, "recoger", true));

    std::cout << "[ITEMS] Pedazos de llave 1 colocados en posiciones fijas:" << std::endl;

    for (int i = 0; i < 3; i++)
    {
        std::cout << "  Pedazo " << (i + 1) << ": "
            << piecePositions[i].x << ", "
            << piecePositions[i].y << ", "
            << piecePositions[i].z << std::endl;
    }
}

void ItemSystem::SpawnKey2At(glm::vec3 position, bool drawModel)
{
    items.push_back(Item(ItemType::KEY2, position, 4.0f, "recoger", drawModel));

    std::cout << "[ITEMS] Llave 2 colocada en: "
        << position.x << ", " << position.y << ", " << position.z
        << " | drawModel=" << drawModel << std::endl;
}

void ItemSystem::SpawnKey3At(glm::vec3 position, bool drawModel)
{
    items.push_back(Item(ItemType::KEY3, position, 4.0f, "recoger", drawModel));

    std::cout << "[ITEMS] Llave 3 colocada en: "
        << position.x << ", " << position.y << ", " << position.z
        << " | drawModel=" << drawModel << std::endl;
}

void ItemSystem::SpawnKey2Random(const std::vector<glm::vec3>& spawnPoints)
{
    if (spawnPoints.empty())
    {
        std::cout << "[ITEMS] ERROR: Lista de spawn points vacia para llave 2." << std::endl;
        return;
    }

    int index = rand() % spawnPoints.size();
    SpawnKey2At(spawnPoints[index], true);
}

void ItemSystem::SpawnKey3Random(const std::vector<glm::vec3>& spawnPoints)
{
    if (spawnPoints.empty())
    {
        std::cout << "[ITEMS] ERROR: Lista de spawn points vacia para llave 3." << std::endl;
        return;
    }

    int index = rand() % spawnPoints.size();
    SpawnKey3At(spawnPoints[index], true);
}

void ItemSystem::AddItem(ItemType type, glm::vec3 position, float pickupRadius)
{
    items.push_back(Item(type, position, pickupRadius, "recoger", true));

    std::cout << "[ITEMS] Item agregado en: "
        << position.x << ", " << position.y << ", " << position.z << std::endl;
}

void ItemSystem::Update(
    float deltaTime,
    const glm::vec3& playerPosition,
    const glm::vec3& cameraFront,
    bool interactPressed,
    bool interactPressedLastFrame,
    AudioManager* audio)
{
    (void)cameraFront;

    bool justPressed = interactPressed && !interactPressedLastFrame;

    for (auto& item : items)
    {
        if (!item.visible)
            continue;

        item.bobTimer += deltaTime;

        glm::vec2 playerXZ(playerPosition.x, playerPosition.z);
        glm::vec2 itemXZ(item.position.x, item.position.z);

        float horizontalDistance = glm::length(playerXZ - itemXZ);
        bool nearEnough = horizontalDistance < item.pickupRadius;

        if (nearEnough && justPressed)
        {
            item.visible = false;

            if (audio && !item.audioOnPickup.empty())
                audio->Play(item.audioOnPickup);

            switch (item.type)
            {
            case ItemType::KEY:
                hasKey = true;
                std::cout << "[ITEMS] Llave 1 recogida!" << std::endl;
                break;

            case ItemType::KEY1_PIECE1:
            case ItemType::KEY1_PIECE2:
            case ItemType::KEY1_PIECE3:
                key1PiecesCollected++;

                std::cout << "[ITEMS] Pedazo de llave 1 recogido: "
                    << key1PiecesCollected << "/3" << std::endl;

                if (key1PiecesCollected >= 3)
                {
                    hasKey = true;
                    std::cout << "[ITEMS] Llave 1 completada! Ya puedes abrir la puerta 1." << std::endl;
                }
                break;

            case ItemType::KEY2:
                hasKey2 = true;
                std::cout << "[ITEMS] Llave 2 recogida!" << std::endl;
                break;

            case ItemType::KEY3:
                hasKey3 = true;
                std::cout << "[ITEMS] Llave 3 recogida!" << std::endl;
                break;

            default:
                std::cout << "[ITEMS] Item recogido." << std::endl;
                break;
            }

            break;
        }
    }
}

void ItemSystem::Draw(
    unsigned int shaderProgram,
    const glm::mat4& view,
    const glm::mat4& projection,
    unsigned int cubeVAO,
    const glm::mat4& baseModelMatrix)
{
    (void)baseModelMatrix;

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "view"),
        1,
        GL_FALSE,
        glm::value_ptr(view)
    );

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "projection"),
        1,
        GL_FALSE,
        glm::value_ptr(projection)
    );

    glBindVertexArray(cubeVAO);

    for (auto& item : items)
    {
        if (!item.visible)
            continue;

        if (!item.drawModel)
            continue;

        glm::mat4 mat = glm::mat4(1.0f);

        mat = glm::translate(mat, item.position);

        float floatingOffset = std::sin(item.bobTimer * 2.3f) * 0.05f;
        mat = glm::translate(mat, glm::vec3(0.0f, floatingOffset, 0.0f));

        mat = glm::rotate(
            mat,
            glm::radians(90.0f),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        mat = glm::scale(mat, glm::vec3(GetItemScale(item.type)));

        glUniformMatrix4fv(
            glGetUniformLocation(shaderProgram, "model"),
            1,
            GL_FALSE,
            glm::value_ptr(mat)
        );

        float pulse = 0.55f + 0.45f * std::sin(item.bobTimer * 5.5f);
        glm::vec3 glowColor = GetGlowColor(item.type) * (0.22f + pulse * 0.55f);

        glUniform3f(
            glGetUniformLocation(shaderProgram, "highlightColor"),
            glowColor.r,
            glowColor.g,
            glowColor.b
        );

        Model* modelToDraw = GetBestModelForItem(item.type);

        if (modelToDraw != nullptr)
        {
            modelToDraw->Draw(shaderProgram);
        }
        else
        {
            // Fallback visible si el OBJ no carga.
            glm::mat4 fallbackMat = glm::mat4(1.0f);
            fallbackMat = glm::translate(fallbackMat, item.position + glm::vec3(0.0f, 0.15f, 0.0f));
            fallbackMat = glm::scale(fallbackMat, glm::vec3(IsKeyPiece(item.type) ? 0.18f : 0.25f));

            glUniformMatrix4fv(
                glGetUniformLocation(shaderProgram, "model"),
                1,
                GL_FALSE,
                glm::value_ptr(fallbackMat)
            );

            glm::vec3 color = GetItemColor(item.type);
            glUniform3f(
                glGetUniformLocation(shaderProgram, "highlightColor"),
                color.r * 0.8f,
                color.g * 0.8f,
                color.b * 0.8f
            );

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glUniform3f(
            glGetUniformLocation(shaderProgram, "highlightColor"),
            0.0f,
            0.0f,
            0.0f
        );
    }
}

void ItemSystem::DrawGlowMarkers(
    unsigned int lampShaderProgram,
    const glm::mat4& view,
    const glm::mat4& projection,
    unsigned int cubeVAO)
{
    glUseProgram(lampShaderProgram);

    glUniformMatrix4fv(
        glGetUniformLocation(lampShaderProgram, "view"),
        1,
        GL_FALSE,
        glm::value_ptr(view)
    );

    glUniformMatrix4fv(
        glGetUniformLocation(lampShaderProgram, "projection"),
        1,
        GL_FALSE,
        glm::value_ptr(projection)
    );

    glBindVertexArray(cubeVAO);

    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);

    for (auto& item : items)
    {
        if (!item.visible)
            continue;

        glm::vec3 baseColor = GetGlowColor(item.type);

        float t = item.bobTimer;
        float pulse = 0.6f + 0.4f * std::sin(t * 6.0f);
        float hover = 0.65f + std::sin(t * 2.0f) * 0.08f;

        glm::vec3 centerPos = item.position + glm::vec3(0.0f, hover, 0.0f);

        // Nucleo tipo luciernaga.
        {
            glm::mat4 coreMat = glm::mat4(1.0f);
            coreMat = glm::translate(coreMat, centerPos);
            coreMat = glm::scale(coreMat, glm::vec3(0.13f + pulse * 0.05f));

            glm::vec3 coreColor = baseColor * (1.6f + pulse * 0.9f);

            glUniformMatrix4fv(
                glGetUniformLocation(lampShaderProgram, "model"),
                1,
                GL_FALSE,
                glm::value_ptr(coreMat)
            );

            glUniform3f(
                glGetUniformLocation(lampShaderProgram, "lampColor"),
                coreColor.r,
                coreColor.g,
                coreColor.b
            );

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Particulas orbitando.
        for (int i = 0; i < 3; ++i)
        {
            float angle = t * (1.8f + i * 0.25f) + i * 2.094f;
            float radius = 0.19f + 0.03f * i;

            glm::vec3 orbitOffset(
                std::cos(angle) * radius,
                std::sin(angle * 1.5f) * 0.08f,
                std::sin(angle) * radius
            );

            glm::vec3 particlePos = centerPos + orbitOffset;

            glm::mat4 particleMat = glm::mat4(1.0f);
            particleMat = glm::translate(particleMat, particlePos);
            particleMat = glm::scale(particleMat, glm::vec3(0.055f + 0.012f * std::sin(t * 7.0f + i)));

            glm::vec3 particleColor = baseColor * (1.8f + pulse);

            glUniformMatrix4fv(
                glGetUniformLocation(lampShaderProgram, "model"),
                1,
                GL_FALSE,
                glm::value_ptr(particleMat)
            );

            glUniform3f(
                glGetUniformLocation(lampShaderProgram, "lampColor"),
                particleColor.r,
                particleColor.g,
                particleColor.b
            );

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    if (depthWasEnabled)
        glEnable(GL_DEPTH_TEST);
}

glm::vec3 ItemSystem::GetItemColor(ItemType type)
{
    switch (type)
    {
    case ItemType::KEY:
    case ItemType::KEY1_PIECE1:
    case ItemType::KEY1_PIECE2:
    case ItemType::KEY1_PIECE3:
        return glm::vec3(1.0f, 0.85f, 0.0f);

    case ItemType::KEY2:
        return glm::vec3(0.2f, 0.6f, 1.0f);

    case ItemType::KEY3:
        return glm::vec3(0.8f, 0.2f, 0.8f);

    default:
        return glm::vec3(1.0f, 1.0f, 1.0f);
    }
}

glm::vec3 ItemSystem::GetGlowColor(ItemType type)
{
    switch (type)
    {
    case ItemType::KEY:
    case ItemType::KEY1_PIECE1:
    case ItemType::KEY1_PIECE2:
    case ItemType::KEY1_PIECE3:
        return glm::vec3(1.0f, 0.92f, 0.25f);

    case ItemType::KEY2:
        return glm::vec3(0.35f, 0.75f, 1.0f);

    case ItemType::KEY3:
        return glm::vec3(0.95f, 0.35f, 1.0f);

    default:
        return glm::vec3(1.0f, 1.0f, 1.0f);
    }
}

float ItemSystem::GetItemScale(ItemType type)
{
    switch (type)
    {
    case ItemType::KEY:
        return 1.0f;

    case ItemType::KEY1_PIECE1:
    case ItemType::KEY1_PIECE2:
    case ItemType::KEY1_PIECE3:
        return KEY1_PIECE_SCALE;

    case ItemType::KEY2:
        return KEY_NORMAL_SCALE;

    case ItemType::KEY3:
        return KEY_NORMAL_SCALE;

    default:
        return 1.0f;
    }
}
