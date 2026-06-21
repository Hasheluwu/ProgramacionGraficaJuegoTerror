#pragma once

#include <glm/glm.hpp>

namespace Raycast
{
    inline bool HitPoint(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDir,
        const glm::vec3& targetPoint,
        float radius)
    {
        glm::vec3 toTarget = targetPoint - rayOrigin;
        float t = glm::dot(toTarget, rayDir);

        if (t < 0.0f)
            return false;

        glm::vec3 closest = rayOrigin + rayDir * t;
        float dist = glm::length(closest - targetPoint);

        return dist < radius;
    }

    inline bool HitPointInRange(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDir,
        const glm::vec3& targetPoint,
        float radius,
        float maxDistance)
    {
        float dist = glm::length(targetPoint - rayOrigin);

        if (dist > maxDistance)
            return false;

        return HitPoint(rayOrigin, rayDir, targetPoint, radius);
    }
}