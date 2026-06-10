#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

// Movimientos de cámara
enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Valores por defecto
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 8.0f;
const float SENSITIVITY = 0.08f;
const float ZOOM = 45.0f;

class Camera
{
public:
    // Atributos principales
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // Ángulos
    float Yaw;
    float Pitch;

    // Opciones
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    Camera(
        glm::vec3 position = glm::vec3(0.0f, 2.0f, 8.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = YAW,
        float pitch = PITCH
    )
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;

        Front = glm::vec3(0.0f, 0.0f, -1.0f);

        MovementSpeed = SPEED;
        MouseSensitivity = SENSITIVITY;
        Zoom = ZOOM;

        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Movimiento con teclado: W, A, S, D
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;

        if (direction == FORWARD)
            Position += GetFlatFront() * velocity;

        if (direction == BACKWARD)
            Position -= GetFlatFront() * velocity;

        if (direction == LEFT)
            Position -= GetFlatRight() * velocity;

        if (direction == RIGHT)
            Position += GetFlatRight() * velocity;
    }

    // Movimiento del mouse
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;

            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        updateCameraVectors();
    }

    // Scroll del mouse
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= yoffset;

        if (Zoom < 1.0f)
            Zoom = 1.0f;

        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }

    // Dirección frontal plana, sin componente Y
    // Esto evita que al mirar hacia arriba o abajo la cámara suba o baje.
    glm::vec3 GetFlatFront() const
    {
        glm::vec3 flatFront = glm::vec3(Front.x, 0.0f, Front.z);

        if (glm::length(flatFront) == 0.0f)
            return glm::vec3(0.0f, 0.0f, -1.0f);

        return glm::normalize(flatFront);
    }

    // Dirección derecha plana, sin componente Y
    glm::vec3 GetFlatRight() const
    {
        glm::vec3 flatRight = glm::vec3(Right.x, 0.0f, Right.z);

        if (glm::length(flatRight) == 0.0f)
            return glm::vec3(1.0f, 0.0f, 0.0f);

        return glm::normalize(flatRight);
    }

private:
    void updateCameraVectors()
    {
        glm::vec3 front;

        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};