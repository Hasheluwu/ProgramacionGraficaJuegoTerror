#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

in vec4 FragPosFlashLight;
in vec4 FragPosLampLight;

uniform sampler2D texture_diffuse1;
uniform vec3 materialColor;
uniform bool hasDiffuseTexture;

uniform vec3 viewPos;
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

#define NUM_LIGHTS 35
uniform PointLight lights[NUM_LIGHTS];

float ShadowCalculation(sampler2D shadowMap, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    float currentDepth = projCoords.z;
    float bias = max(0.0015 * (1.0 - abs(dot(normal, lightDir))), 0.0005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -2; x <= 2; ++x)
        for (int y = -2; y <= 2; ++y) {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            if (currentDepth - bias > closestDepth) shadow += 1.0;
        }
    return clamp(shadow / 25.0, 0.0, 1.0);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor, int lightIndex)
{
    float distance = length(light.position - fragPos);
    if (distance > light.range) return vec3(0.0);

    vec3 lightDir = normalize(light.position - fragPos);
    float diff = clamp(abs(dot(normal, lightDir)), 0.0, 1.0);
    float spec = pow(max(dot(viewDir, reflect(-lightDir, normal)), 0.0), 16.0);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    float rangeFactor = 1.0 - smoothstep(light.range * 0.70, light.range, distance);

    vec3 ambient  = 0.04 * baseColor * light.color;
    vec3 diffuse  = diff * baseColor * light.color;
    vec3 specular = vec3(0.05) * spec * light.color;

    float shadow = 0.0;
    float spotFactor = 1.0;

    if (lightIndex == 26) {
        vec3 fromLampToFrag = normalize(fragPos - light.position);
        float theta = dot(fromLampToFrag, normalize(vec3(0.0, -1.0, 0.0)));
        float innerCutoff = cos(radians(48.0));
        float outerCutoff = cos(radians(68.0));
        spotFactor = clamp((theta - outerCutoff) / (innerCutoff - outerCutoff), 0.0, 1.0);
        if (lampShadowEnabled && spotFactor > 0.01) {
            shadow = clamp(ShadowCalculation(lampShadowMap, FragPosLampLight, normal, lightDir) * 1.15, 0.0, 0.85);
        }
    }

    return (ambient + (1.0 - shadow) * (diffuse + specular)) * attenuation * light.intensity * rangeFactor * spotFactor;
}

void main()
{
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    if (hasDiffuseTexture && texColor.a < 0.1) discard;

    vec3 baseColor = materialColor;
    if (hasDiffuseTexture) {
        baseColor = texColor.rgb;
        if (baseColor.r < 0.025 && baseColor.g < 0.025 && baseColor.b < 0.025)
            baseColor = materialColor;
    }
    if (baseColor.r < 0.02 && baseColor.g < 0.02 && baseColor.b < 0.02)
        baseColor = vec3(0.65, 0.65, 0.65);

    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result  = baseColor * 0.08;

    for (int i = 0; i < NUM_LIGHTS; i++)
        result += CalcPointLight(lights[i], norm, FragPos, viewDir, baseColor, i);

    if (flashlightOn) {
        vec3 lDir  = normalize(flashlightPos - FragPos);
        float theta = dot(lDir, normalize(-flashlightDir));
        float eps   = cos(radians(16.0)) - cos(radians(28.0));
        float inten = clamp((theta - cos(radians(28.0))) / eps, 0.0, 1.0);
        float dist2 = length(flashlightPos - FragPos);
        float att   = 1.0 / (1.0 + 0.014 * dist2 + 0.0032 * dist2 * dist2);
        float diff2 = clamp(abs(dot(norm, lDir)), 0.0, 1.0);
        if (dist2 < 55.0 && inten > 0.01) {
            float rangeFactor  = 1.0 - smoothstep(55.0 * 0.75, 55.0, dist2);
            float flashShadow  = clamp(ShadowCalculation(flashShadowMap, FragPosFlashLight, norm, lDir) * 1.75, 0.0, 1.0);
            vec3 flashResult   = baseColor * vec3(1.0, 0.92, 0.72) * diff2 * att * inten * rangeFactor * 2.0;
            result += (1.0 - flashShadow * 0.98) * flashResult;
        }
    }

    float dist      = length(viewPos - FragPos);
    float fogFactor = clamp((45.0 - dist) / (45.0 - 4.0), 0.0, 1.0);
    FragColor = vec4(mix(vec3(0.005, 0.005, 0.012), result, fogFactor), 1.0);
}