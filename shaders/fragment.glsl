#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec3 viewPos;
uniform float time;

uniform bool flashlightOn;
uniform vec3 flashlightPos;
uniform vec3 flashlightDir;
uniform float brightness;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
};

#define NUM_LIGHTS 35
uniform PointLight lights[NUM_LIGHTS];

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 texColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 ambient  = 0.1 * texColor * light.color;
    vec3 diffuse  = diff * texColor * light.color;
    vec3 specular = vec3(0.3) * spec * light.color;

    return (ambient + diffuse + specular) * attenuation * light.intensity;
}

void main()
{
    vec3 texColor = texture(texture_diffuse1, TexCoords).rgb;
    vec3 norm     = normalize(Normal);
    vec3 viewDir  = normalize(viewPos - FragPos);

    // Luces del techo
    vec3 result = vec3(0.0);
    for (int i = 0; i < NUM_LIGHTS; i++)
        result += CalcPointLight(lights[i], norm, FragPos, viewDir, texColor);

    // Linterna (tecla F)
    if (flashlightOn)
    {
        vec3 lDir   = normalize(flashlightPos - FragPos);
        float theta = dot(lDir, normalize(-flashlightDir));
        float cutoff = cos(radians(16.0));
        float outer  = cos(radians(20.0));
        float eps    = cutoff - outer;
        float inten  = clamp((theta - outer) / eps, 0.0, 1.0);

        float dist2 = length(flashlightPos - FragPos);
        float att   = 1.0 / (1.0 + 0.05 * dist2 + 0.025 * dist2 * dist2);

        float diff2 = max(dot(norm, lDir), 0.0);
        result += texColor * vec3(0.9, 0.85, 0.7) * diff2 * att * inten * 2.0;
    }

    // Niebla
    float fogStart  = 5.0;
    float fogEnd    = 60.0;
    vec3  fogColor  = vec3(0.01, 0.01, 0.02);
    float dist      = length(viewPos - FragPos);
    float fogFactor = clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);
    vec3 finalColor = mix(fogColor, result, fogFactor);

    

FragColor.rgb *= brightness;

    FragColor = vec4(finalColor, 1.0);
}
