#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

out vec4 FragPosFlashLightSpace;
out vec4 FragPosLampLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 flashLightSpaceMatrix;
uniform mat4 lampLightSpaceMatrix;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);

    FragPos = worldPos.xyz;
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;

    FragPosFlashLightSpace = flashLightSpaceMatrix * worldPos;
    FragPosLampLightSpace = lampLightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}