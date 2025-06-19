#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec3 fragTexCoord;

struct PointLight
{
    vec4 position; // ignore w
    vec4 color;    // w is intensity
};

// Set and binding should match the descriptor set layout
layout(set = 0, binding = 0) uniform GlobalUBO
{
    uint textureID;
    mat4 projection;
    mat4 view;
    vec4 ambientLightColor; // w is intensity
    PointLight pointLight;
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
    gl_Position = ubo.projection * ubo.view * positionWorld;

    fragNormalWorld = normalize(mat3(pushdata.normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;
    fragColor = color;
    fragTexCoord = vec3(uv, ubo.textureID);
}

