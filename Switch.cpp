#include "Switch.h"

LightSwitch::LightSwitch(
    const std::string& basePath,
    const std::string& rockerPath,
    const glm::vec3& position,
    const std::vector<int>& lampIndices,
    float interactionDistance,
    float lookRadius,
    bool  startsOn,
    float wallRotation,
    float extraRotationX)
    : baseModel(basePath),
    rockerModel(rockerPath),
    position(position),
    lampIndices(lampIndices),
    interactionDistance(interactionDistance),
    lookRadius(lookRadius),
    isOn(startsOn),
    currentAngle(startsOn ? onAngle : offAngle),
    wallRotation(wallRotation),
    extraRotationX(extraRotationX)
{
    std::cout << "[SWITCH] Cargado en ("
        << position.x << ", " << position.y << ", " << position.z
        << ") controla " << lampIndices.size() << " lampara(s)." << std::endl;
}

void LightSwitch::Update(
    float             deltaTime,
    const glm::vec3& playerPosition,
    const glm::vec3& cameraFront,
    bool              interactPressed,
    bool              interactPressedLastFrame,
    AudioManager* audio,
    bool              lampEnabled[])
{
    isBeingLookedAt = IsPlayerLooking(playerPosition, cameraFront);

    bool justPressed = interactPressed && !interactPressedLastFrame;

    if (justPressed && isBeingLookedAt)
    {
        isOn = !isOn;

        for (int idx : lampIndices)
            lampEnabled[idx] = isOn;

        if (audio)
            audio->Play("abrir_puerta");

        std::cout << "[SWITCH] " << (isOn ? "Encendido" : "Apagado") << std::endl;
    }

    UpdateAnimation(deltaTime);
}

void LightSwitch::UpdateAnimation(float deltaTime)
{
    float targetAngle = isOn ? onAngle : offAngle;
    float step = switchSpeed * deltaTime;

    if (currentAngle < targetAngle)
    {
        currentAngle += step;
        if (currentAngle > targetAngle) currentAngle = targetAngle;
    }
    else if (currentAngle > targetAngle)
    {
        currentAngle -= step;
        if (currentAngle < targetAngle) currentAngle = targetAngle;
    }
}

void LightSwitch::Draw(unsigned int shaderProgram, const glm::mat4& baseModelMatrix)
{
    glm::mat4 baseMat = baseModelMatrix;
    baseMat = glm::translate(baseMat, position);
    baseMat = glm::rotate(baseMat, glm::radians(wallRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    baseMat = glm::rotate(baseMat, glm::radians(extraRotationX), glm::vec3(0.0f, 0.0f, 1.0f));
    baseMat = glm::scale(baseMat, glm::vec3(6.0f));

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(baseMat)
    );
    baseModel.Draw(shaderProgram);

    glm::mat4 rockerMat = glm::rotate(baseMat, glm::radians(currentAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(rockerMat)
    );
    rockerModel.Draw(shaderProgram);
}

bool LightSwitch::IsPlayerLooking(
    const glm::vec3& playerPosition,
    const glm::vec3& cameraFront) const
{
    glm::vec3 rayDir = glm::normalize(cameraFront);
    glm::vec3 visualPos = position + glm::vec3(0.0f, -1.0f, 0.0f);
    return Raycast::HitPointInRange(
        playerPosition,
        rayDir,
        visualPos,
        lookRadius,
        interactionDistance
    );
}