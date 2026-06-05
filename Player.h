#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Camera.h"

const float PLAYER_SPEED = 5.0f;
const float PLAYER_SPRINT_SPEED = 10.0f;
const float PLAYER_CROUCH_MULT = 0.5f;

const float HEIGHT_STAND = 2.2f;
const float HEIGHT_CROUCH = 1.3f;
const float CROUCH_LERP_SPEED = 8.0f;

const float JUMP_FORCE = 6.0f;
const float GRAVITY = 18.0f;

// Suelo: Plane en Y=0.4, modelo transladado -1.0 en main -> 0.4 - 1.0 = -0.6
const float FLOOR_Y = -0.6f;

struct FlashlightData
{
    bool      on;
    glm::vec3 position;
    glm::vec3 direction;
};

class Player
{
public:
    Camera        camera;

    // feetY = posicion real de los pies en el mundo
    // camera.Position.y = feetY + eyeHeight  (se actualiza cada frame)
    float feetY;

    bool  isGrounded;
    bool  isCrouching;
    float verticalVelocity;
    float eyeHeight;        // altura animada del ojo (lerp)

    FlashlightData flashlight;

    Player(glm::vec3 startPos = glm::vec3(0.0f, 2.0f, 8.0f));

    void ProcessInput(GLFWwindow* window, float deltaTime);
    void UpdatePhysics(float deltaTime);
    void UpdateFlashlight();

private:
    bool prevSpace;
    bool prevTab;
    bool prevFlashlight;

    void move(Camera_Movement dir, float deltaTime, bool sprinting);
    void jump();
    void setCrouch(bool crouch);
};