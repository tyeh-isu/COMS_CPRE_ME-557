#version 450

/* only location and type (vec3) matter, the name doesn't need to match */

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;

layout (location = 0) out vec4 outColor;

struct PointLight
{
    vec4 position; // ignore w
    vec4 color;    // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUBO
{
    mat4 projection;
    mat4 view;
    vec4 ambientLightColor; // w is intensity
    PointLight pointLight;
} ubo;

layout(push_constant) uniform Pushdata
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} pushdata;


void main()
{
    vec3 directionToLight = ubo.pointLight.position.xyz - fragPosWorld;
    float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared

    vec3 lightColor = ubo.pointLight.color.xyz * ubo.pointLight.color.w * attenuation;
    vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 diffuseLight = lightColor * max(dot(normalize(fragNormalWorld), normalize(directionToLight)), 0); // make sure to normalize again for fragNormalWorld
    outColor = vec4((diffuseLight + ambientLight) * fragColor, 1.0);
}

