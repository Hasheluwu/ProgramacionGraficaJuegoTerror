#include "Player.h"
#include <glm/glm.hpp>

Player::Player(glm::vec3 startPos)
    : camera(startPos),
    feetY(startPos.y - HEIGHT_STAND),   // pies = posicion inicial - altura de ojos
    isGrounded(false),
    isCrouching(false),
    verticalVelocity(0.0f),
    eyeHeight(HEIGHT_STAND),
    prevSpace(false),
    prevTab(false),
    prevFlashlight(false)
{
    flashlight.on = false;
    flashlight.position = startPos;
    flashlight.direction = camera.Front;
}

// --------------------------------------------------
void Player::ProcessInput(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool sprinting = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) move(FORWARD, deltaTime, sprinting);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) move(BACKWARD, deltaTime, sprinting);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) move(LEFT, deltaTime, sprinting);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) move(RIGHT, deltaTime, sprinting);

    bool spaceNow = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
    if (spaceNow && !prevSpace) jump();
    prevSpace = spaceNow;

    bool tabNow = (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);
    if (tabNow && !prevTab) setCrouch(!isCrouching);
    prevTab = tabNow;

    bool flashNow = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS);
    if (flashNow && !prevFlashlight) flashlight.on = !flashlight.on;
    prevFlashlight = flashNow;
}

// --------------------------------------------------
void Player::UpdatePhysics(float deltaTime)
{
    // --- Gravedad sobre los pies ---
    if (!isGrounded)
        verticalVelocity -= GRAVITY * deltaTime;

    feetY += verticalVelocity * deltaTime;

    // --- Colision con el suelo (pies tocan FLOOR_Y) ---
    if (feetY <= FLOOR_Y)
    {
        feetY = FLOOR_Y;
        verticalVelocity = 0.0f;
        isGrounded = true;
    }

    // --- Altura objetivo del ojo segun estado ---
    float targetEye = isCrouching ? HEIGHT_CROUCH : HEIGHT_STAND;

    // --- Lerp suave de la camara hacia la altura objetivo ---
    float t = glm::clamp(CROUCH_LERP_SPEED * deltaTime, 0.0f, 1.0f);
    eyeHeight = glm::mix(eyeHeight, targetEye, t);

    // --- Aplicar a la camara: pies + altura del ojo ---
    // XZ viene de move(), solo actualizamos Y
    camera.Position.y = feetY + eyeHeight;
}

// --------------------------------------------------
void Player::UpdateFlashlight()
{
    flashlight.position = camera.Position;
    flashlight.direction = camera.Front;
}

// --------------------------------------------------
void Player::move(Camera_Movement dir, float deltaTime, bool sprinting)
{
    float speed = sprinting ? PLAYER_SPRINT_SPEED : PLAYER_SPEED;
    if (isCrouching) speed *= PLAYER_CROUCH_MULT;

    float vel = speed * deltaTime;

    glm::vec3 flatFront = camera.GetFlatFront();
    glm::vec3 flatRight = camera.GetFlatRight();

    if (dir == FORWARD)  camera.Position += flatFront * vel;
    if (dir == BACKWARD) camera.Position -= flatFront * vel;
    if (dir == LEFT)     camera.Position -= flatRight * vel;
    if (dir == RIGHT)    camera.Position += flatRight * vel;
}

void Player::jump()
{
    if (isGrounded && !isCrouching)
    {
        verticalVelocity = JUMP_FORCE;
        isGrounded = false;
    }
}

void Player::setCrouch(bool crouch)
{
    if (!isGrounded) return;
    isCrouching = crouch;
}