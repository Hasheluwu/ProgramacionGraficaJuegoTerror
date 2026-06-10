#include "Player.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Player::Player(glm::vec3 startPos)
    : camera(startPos),
    feetY(startPos.y - HEIGHT_STAND),
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

    bool sprinting =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        move(FORWARD, deltaTime, sprinting);

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        move(BACKWARD, deltaTime, sprinting);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        move(LEFT, deltaTime, sprinting);

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        move(RIGHT, deltaTime, sprinting);

    bool spaceNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    if (spaceNow && !prevSpace)
        jump();

    prevSpace = spaceNow;

    bool tabNow =
        glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;

    if (tabNow && !prevTab)
        setCrouch(!isCrouching);

    prevTab = tabNow;

    bool flashNow = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;

    if (flashNow && !prevFlashlight)
        flashlight.on = !flashlight.on;

    prevFlashlight = flashNow;
}

// --------------------------------------------------
void Player::UpdatePhysics(float deltaTime)
{
    if (!isGrounded)
        verticalVelocity -= GRAVITY * deltaTime;

    feetY += verticalVelocity * deltaTime;

    if (feetY <= FLOOR_Y)
    {
        feetY = FLOOR_Y;
        verticalVelocity = 0.0f;
        isGrounded = true;
    }

    float targetEye = isCrouching ? HEIGHT_CROUCH : HEIGHT_STAND;

    float t = glm::clamp(CROUCH_LERP_SPEED * deltaTime, 0.0f, 1.0f);
    eyeHeight = glm::mix(eyeHeight, targetEye, t);

    camera.Position.y = feetY + eyeHeight;
}

// --------------------------------------------------
void Player::UpdateFlashlight()
{
    glm::vec3 front = glm::normalize(camera.Front);

    glm::vec3 right = glm::normalize(
        glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f))
    );

    // La linterna ya no nace en los ojos.
    // Ahora nace un poco adelante, a la derecha y abajo.
    flashlight.position =
        camera.Position +
        front * 0.45f +
        right * 0.18f +
        glm::vec3(0.0f, -0.18f, 0.0f);

    // Apunta hacia el centro de donde mira la camara.
    glm::vec3 target = camera.Position + front * 10.0f;

    flashlight.direction = glm::normalize(target - flashlight.position);
}

// --------------------------------------------------
void Player::move(Camera_Movement dir, float deltaTime, bool sprinting)
{
    float speed = sprinting ? PLAYER_SPRINT_SPEED : PLAYER_SPEED;

    if (isCrouching)
        speed *= PLAYER_CROUCH_MULT;

    float vel = speed * deltaTime;

    glm::vec3 flatFront = camera.GetFlatFront();
    glm::vec3 flatRight = camera.GetFlatRight();

    if (dir == FORWARD)
        camera.Position += flatFront * vel;

    if (dir == BACKWARD)
        camera.Position -= flatFront * vel;

    if (dir == LEFT)
        camera.Position -= flatRight * vel;

    if (dir == RIGHT)
        camera.Position += flatRight * vel;
}

// --------------------------------------------------
void Player::jump()
{
    if (isGrounded && !isCrouching)
    {
        verticalVelocity = JUMP_FORCE;
        isGrounded = false;
    }
}

// --------------------------------------------------
void Player::setCrouch(bool crouch)
{
    if (!isGrounded)
        return;

    isCrouching = crouch;
}