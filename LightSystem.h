#pragma once

#include <glm/glm.hpp>

enum LightState
{
    STABLE,
    FLICKER,
    BLACKOUT,
    FLICKER_AND_BLACKOUT
};

struct LightEvent
{
    LightState state;
    float duration;
};

#define NUM_LAMPS 37
#define NUM_EVENTS 4

class LightSystem
{
public:
    glm::vec3 lampPositions[NUM_LAMPS];
    float flickerOffset[NUM_LAMPS];
    float intensities[NUM_LAMPS];
    bool lampEnabled[NUM_LAMPS];

    LightEvent eventPool[NUM_EVENTS];

    float eventTimer = 0.0f;
    LightEvent activeEvent;
    LightState lastState = STABLE;

    bool lightsFlickering = false;

    LightSystem();

    void Update(float deltaTime, float currentFrame, glm::vec3 cameraPos);

private:
    void pickNextEvent();
};