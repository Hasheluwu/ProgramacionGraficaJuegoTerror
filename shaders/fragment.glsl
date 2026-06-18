#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

in vec4 FragPosFlashLightSpace;
in vec4 FragPosLampLightSpace;

uniform sampler2D texture_diffuse1;

uniform vec3 materialColor;
uniform bool hasDiffuseTexture;

uniform vec3 viewPos;
uniform float time;

uniform bool flashlightOn;
uniform vec3 flashlightPos;
uniform vec3 flashlightDir;

uniform sampler2D flashShadowMap;
uniform sampler2D lampShadowMap;

uniform bool lampShadowEnabled;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float range;
};

#define NUM_LIGHTS 37
uniform PointLight lights[NUM_LIGHTS];

float ShadowCalculation(sampler2D shadowMap, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;

    float normalDotLight = abs(dot(normal, lightDir));
    float bias = max(0.0015 * (1.0 - normalDotLight), 0.0005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;

            if (currentDepth - bias > closestDepth)
                shadow += 1.0;
        }
    }

    shadow /= 25.0;

    return clamp(shadow, 0.0, 1.0);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor, int lightIndex)
{
    float distance = length(light.position - fragPos);

    if (distance > light.range)
    {
        return vec3(0.0);
    }

    vec3 lightDir = normalize(light.position - fragPos);

    float diff = abs(dot(normal, lightDir));
    diff = clamp(diff, 0.0, 1.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);

    float attenuation = 1.0 / (
        light.constant +
        light.linear * distance +
        light.quadratic * distance * distance
    );

    float rangeFactor = 1.0 - smoothstep(light.range * 0.70, light.range, distance);

    vec3 ambient = 0.04 * baseColor * light.color;
    vec3 diffuse = diff * baseColor * light.color;
    vec3 specular = vec3(0.05) * spec * light.color;

    float shadow = 0.0;
    float spotFactor = 1.0;

    // Lampara 26 como foco hacia abajo.
    if (lightIndex == 26)
    {
        vec3 lampDirection = normalize(vec3(0.0, -1.0, 0.0));
        vec3 fromLampToFrag = normalize(fragPos - light.position);

        float theta = dot(fromLampToFrag, lampDirection);

        float innerCutoff = cos(radians(48.0));
        float outerCutoff = cos(radians(68.0));

        spotFactor = clamp((theta - outerCutoff) / (innerCutoff - outerCutoff), 0.0, 1.0);

        if (lampShadowEnabled && spotFactor > 0.01)
        {
            shadow = ShadowCalculation(lampShadowMap, FragPosLampLightSpace, normal, lightDir);
            shadow = clamp(shadow * 1.15, 0.0, 0.85);
        }
    }

    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

    return lighting * attenuation * light.intensity * rangeFactor * spotFactor;
}

void main()
{
    vec3 texSample = texture(texture_diffuse1, TexCoords).rgb;
    vec3 baseColor = materialColor;

    if (hasDiffuseTexture)
    {
        baseColor = texSample;

        if (texSample.r < 0.025 && texSample.g < 0.025 && texSample.b < 0.025)
        {
            baseColor = materialColor;
        }
    }

    if (baseColor.r < 0.02 && baseColor.g < 0.02 && baseColor.b < 0.02)
    {
        baseColor = vec3(0.65, 0.65, 0.65);
    }

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = baseColor * 0.08;

    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        result += CalcPointLight(lights[i], norm, FragPos, viewDir, baseColor, i);
    }

    // --------------------------
    // Linterna con sombra
    // --------------------------
    if (flashlightOn)
    {
        vec3 lDir = normalize(flashlightPos - FragPos);

        float theta = dot(lDir, normalize(-flashlightDir));

        float cutoff = cos(radians(16.0));
        float outer = cos(radians(28.0));
        float eps = cutoff - outer;

        float inten = clamp((theta - outer) / eps, 0.0, 1.0);

        float dist2 = length(flashlightPos - FragPos);

        float att = 1.0 / (1.0 + 0.014 * dist2 + 0.0032 * dist2 * dist2);

        float diff2 = abs(dot(norm, lDir));
        diff2 = clamp(diff2, 0.0, 1.0);

        float flashlightRange = 55.0;

        if (dist2 < flashlightRange && inten > 0.01)
        {
            float flashlightRangeFactor = 1.0 - smoothstep(flashlightRange * 0.75, flashlightRange, dist2);

            float flashShadow = ShadowCalculation(flashShadowMap, FragPosFlashLightSpace, norm, lDir);

            flashShadow = clamp(flashShadow * 1.75, 0.0, 1.0);

            vec3 flashDiffuse = baseColor * vec3(1.0, 0.92, 0.72) * diff2;
            vec3 flashResult = flashDiffuse * att * inten * flashlightRangeFactor * 2.0;

            result += (1.0 - flashShadow * 0.98) * flashResult;
        }
    }

    // --------------------------
    // Niebla / tinieblas
    // --------------------------
    float fogStart = 4.0;
    float fogEnd = 45.0;
    vec3 fogColor = vec3(0.005, 0.005, 0.012);

    float dist = length(viewPos - FragPos);
    float fogFactor = clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);

    vec3 finalColor = mix(fogColor, result, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
