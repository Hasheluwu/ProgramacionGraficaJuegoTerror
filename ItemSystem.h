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
    KEY,
    KEY2,
    KEY3,
};

struct Item {
    ItemType    type;
    glm::vec3   position;
    bool        visible;
    float       bobTimer;
    float       rotation;
    float       pickupRadius;
    float       lookRadius;
    std::string audioOnPickup;

    Item(ItemType type, glm::vec3 position,
        float pickupRadius = 2.0f, std::string audio = "recoger")
        : type(type), position(position),
        visible(true), bobTimer(0.0f), rotation(0.0f),
        pickupRadius(pickupRadius), lookRadius(0.06f),
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

    ItemSystem();
    ~ItemSystem();

    void SpawnKeyRandom(const std::vector<glm::vec3>& spawnPoints);
	void SpawnKey2Random(const std::vector<glm::vec3>& spawnPoints);
	void SpawnKey3Random(const std::vector<glm::vec3>& spawnPoints);

    void AddItem(ItemType type, glm::vec3 position, float pickupRadius = 2.5f);

    void Update(
        float deltaTime,
        const glm::vec3& playerPosition,
        const glm::vec3& cameraFront,
        bool interactPressed,
        bool interactPressedLastFrame,
        AudioManager* audio
    );

    // shaderProgram debe ser el shader NORMAL de la escena (con iluminacion),
    // NO el lampShader — para que la llave reciba luz y muestre sus texturas.
    // baseModelMatrix: la misma mat que usas para el sotano (con offset -1 en Y).
    void Draw(
        unsigned int     shaderProgram,
        const glm::mat4& view,
        const glm::mat4& projection,
        unsigned int     cubeVAO,
        const glm::mat4& baseModelMatrix = glm::mat4(1.0f)
    );

private:
    Model* keyModel = nullptr;
    Model* key2Model = nullptr;
    Model* key3Model = nullptr;

    glm::vec3 GetItemColor(ItemType type);
    float     GetItemScale(ItemType type);
};