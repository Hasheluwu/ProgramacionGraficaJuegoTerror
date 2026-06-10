#include "Door.h"

Door::Door(
    const std::string& modelPath,
    const glm::vec3& hingePosition,
    const glm::vec3& interactionPosition,
    float closedAngle,
    float openAngle,
    float openSpeed,
    float interactionDistance
)
    : model(modelPath),
    hingePosition(hingePosition),
    interactionPosition(interactionPosition),
    isOpen(false),
    currentAngle(closedAngle),
    closedAngle(closedAngle),
    openAngle(openAngle),
    openSpeed(openSpeed),
    interactionDistance(interactionDistance)
{
    std::cout << "Puerta cargada: " << modelPath << std::endl;
}

void Door::Update(
    float deltaTime,
    const glm::vec3& playerPosition,
    bool interactPressed,
    bool interactPressedLastFrame,
    AudioManager* audio
)
{
    float distance = glm::length(playerPosition - interactionPosition);

    if (interactPressed && !interactPressedLastFrame)
    {
        std::cout << "Distancia a puerta: " << distance << std::endl;
    }

    if (interactPressed && !interactPressedLastFrame && distance <= interactionDistance)
    {
        isOpen = !isOpen;

        // Mismo sonido para abrir y cerrar.
        if (audio != nullptr)
        {
            audio->Play("abrir_puerta");
        }

        if (isOpen)
            std::cout << "Puerta abierta" << std::endl;
        else
            std::cout << "Puerta cerrada" << std::endl;
    }

    UpdateAnimation(deltaTime);
}

void Door::UpdateAnimation(float deltaTime)
{
    float targetAngle = isOpen ? openAngle : closedAngle;
    float step = openSpeed * deltaTime;

    if (currentAngle < targetAngle)
    {
        currentAngle += step;

        if (currentAngle > targetAngle)
            currentAngle = targetAngle;
    }
    else if (currentAngle > targetAngle)
    {
        currentAngle -= step;

        if (currentAngle < targetAngle)
            currentAngle = targetAngle;
    }
}

void Door::Draw(unsigned int shaderProgram, const glm::mat4& baseModelMatrix)
{
    /*
        La puerta debe estar exportada desde Blender en su posición real del mapa.
        hingePosition es el punto de bisagra en coordenadas del mundo.
    */

    glm::mat4 doorMatrix = baseModelMatrix;

    doorMatrix = glm::translate(doorMatrix, hingePosition);

    doorMatrix = glm::rotate(
        doorMatrix,
        glm::radians(currentAngle),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    doorMatrix = glm::translate(doorMatrix, -hingePosition);

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "model"),
        1,
        GL_FALSE,
        glm::value_ptr(doorMatrix)
    );

    model.Draw(shaderProgram);
}

bool Door::IsOpen() const
{
    return isOpen;
}

float Door::GetCurrentAngle() const
{
    return currentAngle;
}