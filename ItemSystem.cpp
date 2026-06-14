#include "ItemSystem.h"
#include "Raycast.h"
#include <cstdlib>

ItemSystem::ItemSystem()
    : hasKey(false), hasKey2(false), hasKey3(false)
{
    try
    {
        keyModel = new Model("Resources/Models/llaves/llave1.obj");
        std::cout << "[ITEMS] Modelo de llave1 cargado correctamente." << std::endl;
    }
    catch (...)
    {
        keyModel = nullptr;
        std::cout << "[ITEMS] ADVERTENCIA: No se pudo cargar llave1.obj, usando cubo." << std::endl;
    }

    try
    {
        key2Model = new Model("Resources/Models/llaves/llave2.obj");
        std::cout << "[ITEMS] Modelo de llave2 cargado correctamente." << std::endl;
    }
    catch (...)
    {
        key2Model = nullptr;
        std::cout << "[ITEMS] ADVERTENCIA: No se pudo cargar llave2.obj, usando cubo." << std::endl;
    }
    
    try
    {
        key3Model = new Model("Resources/Models/llaves/llave3.obj");
        std::cout << "[ITEMS] Modelo de llave3 cargado correctamente." << std::endl;
    }
    catch (...)
    {
        key3Model = nullptr;
        std::cout << "[ITEMS] ADVERTENCIA: No se pudo cargar llave3.obj, usando cubo." << std::endl;
    }
}

ItemSystem::~ItemSystem()
{
    delete keyModel;
    delete key2Model;
    delete key3Model;
    keyModel = nullptr;
    key2Model = nullptr;
    key3Model = nullptr;
}

void ItemSystem::SpawnKeyRandom(const std::vector<glm::vec3>& spawnPoints)
{
    if (spawnPoints.empty())
    {
        std::cout << "[ITEMS] ERROR: Lista de spawn points vacia." << std::endl;
        return;
    }

    int       index = rand() % spawnPoints.size();
    glm::vec3 pos = spawnPoints[index];

    items.push_back(Item(ItemType::KEY, pos, 2.5f, "recoger"));

    std::cout << "[ITEMS] Llave spawneada en: "
        << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
}

void ItemSystem::SpawnKey2Random(const std::vector<glm::vec3>& spawnPoints)
{
    if (spawnPoints.empty())
    {
        std::cout << "[ITEMS] ERROR: Lista de spawn points vacia (llave 2)." << std::endl;
        return;
    }

    int       index = rand() % spawnPoints.size();
    glm::vec3 pos = spawnPoints[index];

    items.push_back(Item(ItemType::KEY2, pos, 2.5f, "recoger"));

    std::cout << "[ITEMS] Llave 2 spawneada en: "
        << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
}
void ItemSystem::SpawnKey3Random(const std::vector<glm::vec3>& spawnPoints)
{
    if (spawnPoints.empty())
    {
        std::cout << "[ITEMS] ERROR: Lista de spawn points vacia (llave 3)." << std::endl;
        return;
    }

    int       index = rand() % spawnPoints.size();
    glm::vec3 pos = spawnPoints[index];

    items.push_back(Item(ItemType::KEY3, pos, 2.5f, "recoger"));

    std::cout << "[ITEMS] Llave 3 spawneada en: "
        << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
}

void ItemSystem::AddItem(ItemType type, glm::vec3 position, float pickupRadius)
{
    items.push_back(Item(type, position, pickupRadius));
    std::cout << "[ITEMS] Item agregado en: "
        << position.x << ", " << position.y << ", " << position.z << std::endl;
}

void ItemSystem::Update(
    float             deltaTime,
    const glm::vec3& playerPosition,
    const glm::vec3& cameraFront,
    bool              interactPressed,
    bool              interactPressedLastFrame,
    AudioManager* audio)
{
    glm::vec3 rayDir = glm::normalize(cameraFront);

    for (auto& item : items)
    {
        if (!item.visible) continue;

        // Animacion minima: solo avanzar el timer (por si lo quieres usar despues)
        item.bobTimer += deltaTime;

        bool nearEnough = glm::length(playerPosition - item.position) < item.pickupRadius;
        bool lookingAt = Raycast::HitPointInRange(
            playerPosition, rayDir,
            item.position, item.lookRadius, item.pickupRadius);
        bool justPressed = interactPressed && !interactPressedLastFrame;

        if (nearEnough && lookingAt && justPressed)
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
        }
    }
}

// ----------------------------------------------------------------
// Draw — usa el shaderProgram que recibe (debe ser el shader normal
//         de la escena para que la llave tenga iluminacion real).
//
// En main, cambia la llamada a:
//   itemSystem.Draw(shader.ID, view, projection, lampVAO, modelMat);
//
// El parametro baseModelMatrix es la misma modelMat que usas para
// el sotano (-1 en Y).  La posicion de spawn ya incluye esa Y,
// asi que el offset se cancela con el translate posterior.
// ----------------------------------------------------------------
void ItemSystem::Draw(
    unsigned int      shaderProgram,
    const glm::mat4& view,
    const glm::mat4& projection,
    unsigned int      cubeVAO,
    const glm::mat4& baseModelMatrix)
{
    glUseProgram(shaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"),
        1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"),
        1, GL_FALSE, glm::value_ptr(projection));

    for (auto& item : items)
    {
        if (!item.visible) continue;

 
        glm::mat4 mat = glm::mat4(1.0f);
        mat = glm::translate(mat, item.position);
        mat = glm::rotate(mat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        mat = glm::scale(mat, glm::vec3(GetItemScale(item.type)));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"),
            1, GL_FALSE, glm::value_ptr(mat));

        if (item.type == ItemType::KEY && keyModel != nullptr)
        {
            keyModel->Draw(shaderProgram);
        }
        else if (item.type == ItemType::KEY2 && key2Model != nullptr)
        {
            key2Model->Draw(shaderProgram);
        }
        else if (item.type == ItemType::KEY3 && key3Model != nullptr)
        {
            key3Model->Draw(shaderProgram);
        }
        else
        {
            // Fallback: cubo dorado si el modelo no cargo
            glm::vec3 color = GetItemColor(item.type);
            // Si el shader tiene "lampColor" (lampShader), lo seteamos;
            // si no existe el uniform, OpenGL lo ignora sin crashear.
            glUniform3f(glGetUniformLocation(shaderProgram, "lampColor"),
                color.r, color.g, color.b);
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
}

glm::vec3 ItemSystem::GetItemColor(ItemType type)
{
    switch (type)
    {
    case ItemType::KEY:  return glm::vec3(1.0f, 0.85f, 0.0f);
    case ItemType::KEY2: return glm::vec3(0.2f, 0.6f, 1.0f);
    case ItemType::KEY3: return glm::vec3(0.8f, 0.2f, 0.8f); 
    default:             return glm::vec3(1.0f, 1.0f, 1.0f);
    }
}

float ItemSystem::GetItemScale(ItemType type)
{
    switch (type)
    {
    case ItemType::KEY:  return 2.8f;
    case ItemType::KEY2: return 2.8f;
    case ItemType::KEY3: return 2.8f;
    default:             return 2.8f;
    }
}