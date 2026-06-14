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
    // true cuando el jugador esta mirando esta puerta (se setea en Update)
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
        float lookRadius = 1.2f
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

    // ---- Getters ----
    bool  IsOpen()          const;
    float GetCurrentAngle() const;
    bool  RequiresKey()     const;

    // Devuelve true si el jugador esta mirando esta puerta y esta cerca
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
    bool  cachedPlayerHasKey = false;   // guardado en Update para usarlo en Draw
    float lookRadius;

    float currentAngle;
    float closedAngle;
    float openAngle;
    float openSpeed;
    float interactionDistance;

    void UpdateAnimation(float deltaTime);
};