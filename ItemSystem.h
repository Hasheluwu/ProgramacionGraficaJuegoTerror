#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <iostream>

#include "AudioManager.h"
#include "Model.h"

enum class ItemType {
    KEY,          // Compatibilidad vieja. Ya NO carga llave1.obj.
    KEY2,
    KEY3,

    KEY1_PIECE1,
    KEY1_PIECE2,
    KEY1_PIECE3
};

struct Item {
    ItemType    type;
    glm::vec3   position;
    bool        visible;
    bool        drawModel;
    float       bobTimer;
    float       rotation;
    float       pickupRadius;
    float       lookRadius;
    std::string audioOnPickup;

    Item(ItemType type, glm::vec3 position,
        float pickupRadius = 4.0f,
        std::string audio = "recoger",
        bool drawModel = true)
        : type(type),
        position(position),
        visible(true),
        drawModel(drawModel),
        bobTimer(0.0f),
        rotation(0.0f),
        pickupRadius(pickupRadius),
        lookRadius(0.25f),
        audioOnPickup(audio)
    {
    }
};

class ItemSystem
{
public:
    std::vector<Item> items;

    bool hasKey;
    bool hasKey2;
    bool hasKey3;

    int key1PiecesCollected;

    ItemSystem();
    ~ItemSystem();

    // Llave 1 completa vieja. Se deja por compatibilidad, pero ya NO carga llave1.obj.
    void SpawnKeyRandom(const std::vector<glm::vec3>& spawnPoints);

    // Llave 2 y 3
    void SpawnKey2At(glm::vec3 position, bool drawModel = true);
    void SpawnKey3At(glm::vec3 position, bool drawModel = true);

    void SpawnKey2Random(const std::vector<glm::vec3>& spawnPoints);
    void SpawnKey3Random(const std::vector<glm::vec3>& spawnPoints);

    // Llave 1 partida
    void SpawnKey1PiecesAt(glm::vec3 basePosition);
    void SpawnKey1PiecesRandom(const std::vector<glm::vec3>& spawnPoints);
    void SpawnKey1PiecesFixedPositions(const std::vector<glm::vec3>& piecePositions);

    void AddItem(ItemType type, glm::vec3 position, float pickupRadius = 4.0f);

    void Update(
        float deltaTime,
        const glm::vec3& playerPosition,
        const glm::vec3& cameraFront,
        bool interactPressed,
        bool interactPressedLastFrame,
        AudioManager* audio
    );

    void Draw(
        unsigned int     shaderProgram,
        const glm::mat4& view,
        const glm::mat4& projection,
        unsigned int     cubeVAO,
        const glm::mat4& baseModelMatrix = glm::mat4(1.0f)
    );

    void DrawGlowMarkers(
        unsigned int     lampShaderProgram,
        const glm::mat4& view,
        const glm::mat4& projection,
        unsigned int     cubeVAO
    );

private:
    Model* key2Model = nullptr;
    Model* key3Model = nullptr;

    Model* key1Piece1Model = nullptr;
    Model* key1Piece2Model = nullptr;
    Model* key1Piece3Model = nullptr;

    glm::vec3 GetItemColor(ItemType type);
    glm::vec3 GetGlowColor(ItemType type);
    float     GetItemScale(ItemType type);

    Model* GetBestModelForItem(ItemType type);
    bool   IsKeyPiece(ItemType type);
};
