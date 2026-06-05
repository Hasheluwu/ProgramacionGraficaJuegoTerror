#include "LightSystem.h"
#include <glm/glm.hpp>
#include <cmath>
#include <cstdlib>   // rand(), srand()
#include <ctime>     // time()

LightSystem::LightSystem()
{
    srand((unsigned int)time(nullptr));

    lampPositions[0] = glm::vec3(-19.272f, 5.3f, 8.804f);
    lampPositions[1] = glm::vec3(-19.272f, 5.3f, 18.282f);
    lampPositions[2] = glm::vec3(29.968f, 5.3f, 8.804f);
    lampPositions[3] = glm::vec3(29.968f, 5.3f, 18.620f);
    lampPositions[4] = glm::vec3(29.968f, 5.3f, 0.310f);
    lampPositions[5] = glm::vec3(20.072f, 5.3f, 0.310f);
    lampPositions[6] = glm::vec3(5.664f, 5.3f, 0.310f);
    lampPositions[7] = glm::vec3(-10.578f, 5.3f, 0.310f);
    lampPositions[8] = glm::vec3(-28.130f, 5.3f, 0.310f);
    lampPositions[9] = glm::vec3(-37.429f, 5.3f, -16.199f);
    lampPositions[10] = glm::vec3(-37.429f, 5.3f, -34.274f);
    lampPositions[11] = glm::vec3(-0.196f, 5.3f, -34.274f);
    lampPositions[12] = glm::vec3(-0.196f, 5.3f, -18.447f);
    lampPositions[13] = glm::vec3(-0.196f, 5.3f, -7.176f);
    lampPositions[14] = glm::vec3(-0.196f, 5.3f, -46.194f);
    lampPositions[15] = glm::vec3(9.496f, 5.3f, -46.194f);
    lampPositions[16] = glm::vec3(-0.065f, 5.3f, -59.881f);
    lampPositions[17] = glm::vec3(-0.065f, 5.3f, -73.111f);
    lampPositions[18] = glm::vec3(21.704f, 5.3f, -87.807f);
    lampPositions[19] = glm::vec3(-0.065f, 5.3f, -87.728f);
    lampPositions[20] = glm::vec3(43.395f, 5.3f, -87.807f);
    lampPositions[21] = glm::vec3(57.777f, 5.3f, -100.518f);
    lampPositions[22] = glm::vec3(-16.412f, 5.3f, -87.807f);
    lampPositions[23] = glm::vec3(-35.431f, 5.3f, -87.807f);
    lampPositions[24] = glm::vec3(-70.469f, 5.3f, -87.807f);
    lampPositions[25] = glm::vec3(-56.126f, 5.3f, -87.807f);
    lampPositions[26] = glm::vec3(-12.805f, 5.3f, -102.474f);
    lampPositions[27] = glm::vec3(42.947f, 5.3f, -108.666f);
    lampPositions[28] = glm::vec3(86.458f, 5.3f, -118.281f);
    lampPositions[29] = glm::vec3(59.732f, 5.3f, -117.466f);
    lampPositions[30] = glm::vec3(102.591f, 5.3f, -117.466f);
    lampPositions[31] = glm::vec3(102.591f, 5.3f, -130.992f);
    lampPositions[32] = glm::vec3(102.591f, 5.3f, -97.748f);
    lampPositions[33] = glm::vec3(129.480f, 5.3f, -97.748f);
    lampPositions[34] = glm::vec3(129.480f, 5.3f, -131.970f);

    for (int i = 0; i < NUM_LAMPS; i++)
        flickerOffset[i] = i * 1.3f;

    for (int i = 0; i < NUM_LAMPS; i++)
        lampEnabled[i] = true;

    lampEnabled[0] = false;
    lampEnabled[1] = false;

    // --- Pool de eventos (puedes ajustar las duraciones) ---
    eventPool[0] = { STABLE,              6.0f };  // todas estables
    eventPool[1] = { FLICKER,             4.0f };  // solo parpadeo
    eventPool[2] = { BLACKOUT,            4.0f };  // solo apagado
    eventPool[3] = { FLICKER_AND_BLACKOUT, 5.0f }; // parpadeo + apagado combinado

    // Arrancar con un evento aleatorio
    activeEvent = eventPool[rand() % NUM_EVENTS];
    eventTimer = 0.0f;
}

// --------------------------------------------------
void LightSystem::pickNextEvent()
{
    // Sortea hasta sacar uno distinto al actual (evita repetir el mismo dos veces seguidas)
    int next;

    if (activeEvent.state == BLACKOUT)
    {
        activeEvent = eventPool[0];  // eventPool[0] = STABLE
        return;
    }

    if (activeEvent.state == FLICKER_AND_BLACKOUT)
    {
        activeEvent = eventPool[0];  // eventPool[0] = STABLE
        return;
    }

    do {
        next = rand() % NUM_EVENTS;
    } while (eventPool[next].state == activeEvent.state);

    lastState = activeEvent.state;
    activeEvent = eventPool[next];
}

// --------------------------------------------------
void LightSystem::Update(float deltaTime, float currentFrame, glm::vec3 cameraPos)
{
    eventTimer += deltaTime;
    if (eventTimer >= activeEvent.duration)
    {
        eventTimer = 0.0f;
        pickNextEvent();
    }

    for (int i = 0; i < NUM_LAMPS; i++)
    {
        float t = currentFrame + flickerOffset[i];
        float flicker = 1.0f;

        switch (activeEvent.state)
        {
        case STABLE:
            flicker = 0.95f + 0.05f * sin(t * 1.5f);
            break;

        case FLICKER:
            flicker = 0.85f + 0.15f * sin(t * 20.0f);
            {
                float glitch = sin(t * 37.0f) * sin(t * 13.0f);
                if (glitch > 0.6f) flicker *= 0.05f;
            }
            break;

        case BLACKOUT:
        {
            float dist = glm::length(cameraPos - lampPositions[i]);
            float nearFactor = glm::clamp(1.0f - dist / 20.0f, 0.0f, 1.0f);
            flicker = 0.02f + 0.03f * nearFactor * sin(t * 5.0f);
        }
        break;

        case FLICKER_AND_BLACKOUT:
        {
            // Parpadeo agresivo que termina casi apagado
            flicker = 0.85f + 0.15f * sin(t * 20.0f);
            float glitch = sin(t * 37.0f) * sin(t * 13.0f);
            if (glitch > 0.6f) flicker *= 0.05f;

            // Ademas se apaga mas segun distancia al jugador
            float dist = glm::length(cameraPos - lampPositions[i]);
            float nearFactor = glm::clamp(1.0f - dist / 25.0f, 0.0f, 1.0f);
            flicker *= (0.3f + 0.7f * nearFactor);
        }
        break;
        }

        intensities[i] = glm::clamp(flicker, 0.0f, 1.0f);

        if (!lampEnabled[i]) intensities[i] = 0.0f;
    }
}