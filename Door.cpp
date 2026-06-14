#include "Door.h"
#include "Raycast.h"

Door::Door(
    const std::string& modelPath,
    const glm::vec3& hingePosition,
    const glm::vec3& interactionPosition,
    float closedAngle,
    float openAngle,
    float openSpeed,
    float interactionDistance,
    bool requiresKey,
    float lookRadius
)
    : model(modelPath),
    hingePosition(hingePosition),
    interactionPosition(interactionPosition),
    isOpen(false),
    requiresKey(requiresKey),
    cachedPlayerHasKey(false),
    lookRadius(lookRadius),
    currentAngle(closedAngle),
    closedAngle(closedAngle),
    openAngle(openAngle),
    openSpeed(openSpeed),
    interactionDistance(interactionDistance)
{
    std::cout << "[PUERTA] Cargada: " << modelPath;
    if (requiresKey) std::cout << " (requiere llave)";
    std::cout << std::endl;
}

void Door::Update(
    float deltaTime,
    const glm::vec3& playerPosition,
    const glm::vec3& cameraFront,
    bool interactPressed,
    bool interactPressedLastFrame,
    AudioManager* audio,
    bool playerHasKey)
{
    // Guardar estado para Draw
    cachedPlayerHasKey = playerHasKey;
    isBeingLookedAt = IsPlayerLooking(playerPosition, cameraFront);

    bool justPressed = interactPressed && !interactPressedLastFrame;

    if (justPressed && isBeingLookedAt)
    {
        if (requiresKey && !playerHasKey && !isOpen)
        {
            // El HUD ya muestra el mensaje, nada mas que hacer aqui
            std::cout << "[PUERTA] Necesitas una llave." << std::endl;
        }
        else
        {
            isOpen = !isOpen;
            if (audio) audio->Play("abrir_puerta");
            std::cout << "[PUERTA] " << (isOpen ? "Abierta" : "Cerrada") << std::endl;
        }
    }

    UpdateAnimation(deltaTime);
}

bool Door::IsPlayerLooking(
    const glm::vec3& playerPosition,
    const glm::vec3& cameraFront) const
{
    glm::vec3 rayDir = glm::normalize(cameraFront);
    return Raycast::HitPointInRange(
        playerPosition,
        rayDir,
        interactionPosition,
        lookRadius,
        interactionDistance
    );
}

void Door::UpdateAnimation(float deltaTime)
{
    float targetAngle = isOpen ? openAngle : closedAngle;
    float step = openSpeed * deltaTime;

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

void Door::Draw(unsigned int shaderProgram, const glm::mat4& baseModelMatrix)
{
    glm::mat4 doorMatrix = baseModelMatrix;
    doorMatrix = glm::translate(doorMatrix, hingePosition);
    doorMatrix = glm::rotate(doorMatrix, glm::radians(currentAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    doorMatrix = glm::translate(doorMatrix, -hingePosition);

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "model"),
        1, GL_FALSE, glm::value_ptr(doorMatrix)
    );
    /*  
    // --- Highlight del pomo ---
    // Verde  = puerta abrible (libre o tiene llave)
    // Naranja = bloqueada (requiere llave y no la tiene)
    // Negro  = no se esta mirando (sin efecto)
    glm::vec3 highlight(0.0f);
    if (isBeingLookedAt)
    {
        bool blocked = requiresKey && !cachedPlayerHasKey && !isOpen;
        if (blocked)
            highlight = glm::vec3(0.9f, 0.3f, 0.05f);   // naranja
        else
            highlight = glm::vec3(0.2f, 0.7f, 0.2f);    // verde
    }
    glUniform3f(glGetUniformLocation(shaderProgram, "highlightColor"),
        highlight.r, highlight.g, highlight.b);*/

    model.Draw(shaderProgram);

    // Limpiar highlight para que el siguiente objeto no lo herede
    //glUniform3f(glGetUniformLocation(shaderProgram, "highlightColor"), 0.0f, 0.0f, 0.0f);
}

bool  Door::IsOpen()          const { return isOpen; }
float Door::GetCurrentAngle() const { return currentAngle; }
bool  Door::RequiresKey()     const { return requiresKey; }