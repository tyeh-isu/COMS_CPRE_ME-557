#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

// Set and binding should match the descriptor set layout
layout(set = 0, binding = 0) uniform GlobalUBO
{
    mat4 projectionViewMatrix;
    vec4 ambientLightColor; // w is intensity
    vec3 lightPosition;
    vec4 lightColor;
} ubo;

// Note 128 bytes can only contain 2 4x4 matrices, so we have run into the limitation
layout(push_constant) uniform Pushdata
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} pushdata;

void main()
{
    vec4 positionWorld = pushdata.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projectionViewMatrix * positionWorld;

    // Note: this ony works in certain condition
    // it is only correct if uniform scaling is applied (sz == sy == sz)
    vec3 normalWorldSpace = normalize(mat3(pushdata.modelMatrix) * normal);

    // Light
    vec3 directionToLight = ubo.lightPosition - positionWorld.xyz;
    float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared

    vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w * attenuation;
    vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;

    vec3 diffuseLight = lightColor * max(dot(normalWorldSpace, normalize(directionToLight)), 0);;

    fragColor = (diffuseLight + ambientLight) * color;
}

