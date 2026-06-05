#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

uniform vec3 lightPos;
uniform vec3 viewPos;

// ¡NUEVA VARIABLE PARA EL TINTE DE COLOR!
uniform vec3 colorTint; 

void main()
{
    vec3 texColor = texture(texture_diffuse1, TexCoords).rgb;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);

    vec3 ambient = 0.2 * texColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

    vec3 specular = vec3(0.5) * spec;

    vec3 result = ambient + diff * texColor + specular;

    // Multiplicamos el resultado final por nuestro colorTint
    FragColor = vec4(result * colorTint, 1.0);
}