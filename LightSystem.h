#pragma once
#include <glm/glm.hpp>

enum LightState { STABLE, FLICKER, BLACKOUT, FLICKER_AND_BLACKOUT };

struct LightEvent {
    LightState state;
    float      duration;
};

#define NUM_LAMPS 35
#define NUM_EVENTS 4   // cuantos tipos distintos hay en el pool

class LightSystem
{
public:
    glm::vec3 lampPositions[NUM_LAMPS];
    float     flickerOffset[NUM_LAMPS];
    float     intensities[NUM_LAMPS];
    bool      lampEnabled[NUM_LAMPS];

    // Pool de eventos posibles (el sistema sortea uno al azar cada vez)
    LightEvent eventPool[NUM_EVENTS];     // el evento anterior (para evitar repetirlo)

    float      eventTimer = 0.0f;
    LightEvent activeEvent;   // el que esta corriendo ahora
    LightState lastState = STABLE;

    LightSystem();
    void Update(float deltaTime, float currentFrame, glm::vec3 cameraPos);

private:
    void pickNextEvent();   // sortea el siguiente sin repetir el actual
};