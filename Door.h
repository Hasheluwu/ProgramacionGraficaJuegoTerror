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
    Door(
        const std::string& modelPath,
        const glm::vec3& hingePosition,
        const glm::vec3& interactionPosition,
        float closedAngle,
        float openAngle,
        float openSpeed,
        float interactionDistance
    );

    void Update(
        float deltaTime,
        const glm::vec3& playerPosition,
        bool interactPressed,
        bool interactPressedLastFrame,
        AudioManager* audio
    );

    void Draw(unsigned int shaderProgram, const glm::mat4& baseModelMatrix);

    bool IsOpen() const;
    float GetCurrentAngle() const;

private:
    Model model;

    glm::vec3 hingePosition;
    glm::vec3 interactionPosition;

    bool isOpen;

    float currentAngle;
    float closedAngle;
    float openAngle;
    float openSpeed;
    float interactionDistance;

    void UpdateAnimation(float deltaTime);
};