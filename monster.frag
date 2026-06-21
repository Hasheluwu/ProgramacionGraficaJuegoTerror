#version 330 core

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D texture_diffuse1;
uniform bool hasDiffuseTexture;
uniform vec3 materialColor;

void main()
{
    if (hasDiffuseTexture)
        FragColor = texture(texture_diffuse1, TexCoords);
    else
        FragColor = vec4(materialColor, 1.0);
}