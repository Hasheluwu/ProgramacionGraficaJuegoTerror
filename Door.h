#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <iostream>

#include "Model.h"
#include "AudioManager.h"

class Door
{
public:
    bool isBeingLookedAt = false;

    Door(
        const std::string& modelPath,
        const glm::vec3& hingePosition,
        const glm::vec3& interactionPosition,
        float closedAngle,
        float openAngle,
        float openSpeed,
        float interactionDistance,
        bool requiresKey = false,
        float lookRadius = 1.2f,
        bool* externalUnlock = nullptr,
        bool* sharedOpenState = nullptr
    );

    void Update(
        float deltaTime,
        const glm::vec3& playerPosition,
        const glm::vec3& cameraFront,
        bool interactPressed,
        bool interactPressedLastFrame,
        AudioManager* audio,
        bool playerHasKey = false
    );

    void Draw(unsigned int shaderProgram, const glm::mat4& baseModelMatrix);

    bool  IsOpen()          const;
    float GetCurrentAngle() const;
    bool  RequiresKey()     const;
    bool  IsUnlocked()      const;
    glm::vec3 GetPosition() const { return hingePosition; }

    bool IsPlayerLooking(
        const glm::vec3& playerPosition,
        const glm::vec3& cameraFront
    ) const;

private:
    Model model;
    glm::vec3 hingePosition;
    glm::vec3 interactionPosition;

    bool  isOpen = false;
    bool  requiresKey = false;
    bool  cachedPlayerHasKey = false;
    float lookRadius;

    float currentAngle;
    float closedAngle;
    float openAngle;
    float openSpeed;
    float interactionDistance;

    bool* externalUnlock = nullptr;
    bool* sharedOpenState = nullptr;

    void UpdateAnimation(float deltaTime);
};