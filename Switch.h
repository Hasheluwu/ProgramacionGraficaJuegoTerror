#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <iostream>

#include "Model.h"
#include "AudioManager.h"
#include "Raycast.h"

class LightSwitch
{
public:
    bool isOn;
    bool isBeingLookedAt = false;

    LightSwitch(
        const std::string& basePath,
        const std::string& rockerPath,
        const glm::vec3& position,
        const std::vector<int>& lampIndices,
        float interactionDistance = 3.0f,
        float lookRadius = 0.5f,
        bool  startsOn = true,
        float wallRotation = 180.0f,
        float extraRotationX = 0.0f
    );

    void Update(
        float             deltaTime,
        const glm::vec3& playerPosition,
        const glm::vec3& cameraFront,
        bool              interactPressed,
        bool              interactPressedLastFrame,
        AudioManager* audio,
        bool              lampEnabled[]
    );

    // Enciende o apaga el switch directamente (usado por el monstruo)
    void SetOn(bool on, bool lampEnabled[]) {
        isOn = on;
        for (int idx : lampIndices)
            lampEnabled[idx] = isOn;
    }

    // Devuelve la posición en el mundo (usada para mapear celda->switch)
    glm::vec3 GetPosition() const { return position; }

    void Draw(unsigned int shaderProgram, const glm::mat4& baseModelMatrix);

    const std::vector<int>& GetLampIndices() const { return lampIndices; }

private:
    Model            baseModel;
    Model            rockerModel;
    glm::vec3        position;
    std::vector<int> lampIndices;

    float interactionDistance;
    float lookRadius;

    float onAngle = 15.0f;
    float offAngle = -15.0f;
    float switchSpeed = 180.0f;
    float wallRotation = 180.0f;
    float extraRotationX = 0.0f;
    float currentAngle = 0.0f;

    void UpdateAnimation(float deltaTime);
    bool IsPlayerLooking(const glm::vec3& playerPosition, const glm::vec3& cameraFront) const;
};