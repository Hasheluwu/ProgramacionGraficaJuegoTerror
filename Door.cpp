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
    float lookRadius,
    bool* externalUnlockFlag,
    bool* sharedOpenFlag
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
    interactionDistance(interactionDistance),
    externalUnlock(externalUnlockFlag),
    sharedOpenState(sharedOpenFlag)
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
    cachedPlayerHasKey = playerHasKey;
    isBeingLookedAt = IsPlayerLooking(playerPosition, cameraFront);

    bool justPressed = interactPressed && !interactPressedLastFrame;

    // Determinar si la puerta está bloqueada
    bool blocked = false;
    if (externalUnlock != nullptr) {
        if (!(*externalUnlock)) blocked = true;
    }
    else if (requiresKey && !playerHasKey && !isOpen) {
        blocked = true;
    }

    if (justPressed && isBeingLookedAt) {
        if (blocked) {
            std::cout << "[PUERTA] Bloqueada." << std::endl;
            if (audio) {
                float dist = glm::length(playerPosition - hingePosition);
                float vol = glm::clamp(1.0f - (dist / 250.0f), 0.0f, 1.0f);
                audio->SetVolume("error", vol * 0.8f);
                audio->Play("error");
            }
        }
        else {
            // Abrir/cerrar
            if (sharedOpenState != nullptr) {
                *sharedOpenState = !(*sharedOpenState);
                isOpen = *sharedOpenState;
            }
            else {
                isOpen = !isOpen;
            }

            // Si se abrió usando una llave, la puerta deja de requerirla para siempre
            if (requiresKey && isOpen && playerHasKey) {
                requiresKey = false;
                std::cout << "[PUERTA] Desbloqueada permanentemente (ya no requiere llave)." << std::endl;
            }

            // Sonido de puerta con volumen por distancia
            if (audio) {
                float dist = glm::length(playerPosition - hingePosition);
                float vol = glm::clamp(1.0f - (dist / 250.0f), 0.0f, 1.0f);
                audio->SetVolume("abrir_puerta", vol * 0.8f);
                audio->Play("abrir_puerta");
            }
            std::cout << "[PUERTA] " << (isOpen ? "Abierta" : "Cerrada") << std::endl;
        }
    }

    // Sincronizar con estado compartido (por si otra hoja lo cambió)
    if (sharedOpenState != nullptr) {
        isOpen = *sharedOpenState;
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

    if (currentAngle < targetAngle) {
        currentAngle += step;
        if (currentAngle > targetAngle) currentAngle = targetAngle;
    }
    else if (currentAngle > targetAngle) {
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

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(doorMatrix));
    model.Draw(shaderProgram);
}

bool Door::IsOpen()          const { return isOpen; }
float Door::GetCurrentAngle() const { return currentAngle; }
bool Door::RequiresKey()     const { return requiresKey; }
bool Door::IsUnlocked()      const {
    if (externalUnlock) return *externalUnlock;
    return !requiresKey;
}

void Door::ForceOpen()
{
    if (isOpen) return;

    if (requiresKey && !cachedPlayerHasKey) {
        std::cout << "[PUERTA] El monstruo no puede forzar una puerta con llave." << std::endl;
        return;
    }
    if (externalUnlock && !(*externalUnlock)) {
        std::cout << "[PUERTA] El monstruo no puede forzar una puerta bloqueada externamente." << std::endl;
        return;
    }

    if (sharedOpenState) {
        *sharedOpenState = true;
        isOpen = true;
    }
    else {
        isOpen = true;
    }
    std::cout << "[PUERTA] Forzada por el monstruo." << std::endl;
}