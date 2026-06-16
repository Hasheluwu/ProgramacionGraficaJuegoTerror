#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4  aWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 flashLightSpaceMatrix;
uniform mat4 lampLightSpaceMatrix;

#define MAX_BONES 100
uniform mat4 boneMatrices[MAX_BONES];

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 FragPosFlashLight;
out vec4 FragPosLampLight;

void main()
{
    // Skinning: combinar hasta 4 huesos (w = 1.0 para incluir traslación)
    mat4 skinMatrix = mat4(0.0);
    for (int i = 0; i < 4; i++) {
        if (aWeights[i] > 0.0)
            skinMatrix += aWeights[i] * boneMatrices[aBoneIDs[i]];
    }
    // Si no tiene pesos, usar identidad
    if (dot(skinMatrix[0], vec4(1.0)) + dot(skinMatrix[1], vec4(1.0)) +
        dot(skinMatrix[2], vec4(1.0)) + dot(skinMatrix[3], vec4(1.0)) < 0.001)
        skinMatrix = mat4(1.0);

    vec4 skinnedPos    = skinMatrix * vec4(aPos,    1.0);   // w = 1.0
    vec4 skinnedNormal = skinMatrix * vec4(aNormal, 0.0);   // w = 0.0 (dirección)

    vec4 worldPos = model * skinnedPos;

    FragPos   = vec3(worldPos);
    Normal    = normalize(mat3(transpose(inverse(model * skinMatrix))) * aNormal);
    TexCoords = aTexCoords;
    FragPosFlashLight = flashLightSpaceMatrix * worldPos;
    FragPosLampLight  = lampLightSpaceMatrix  * worldPos;

    gl_Position = projection * view * worldPos;
}