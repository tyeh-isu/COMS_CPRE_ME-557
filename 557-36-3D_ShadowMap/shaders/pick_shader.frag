#version 450

/* only location and type (vec3) matter, the name doesn't need to match */

layout(location = 0) flat in float inID; // flat means no interpolation
layout(location = 0) out float outColor;

// Set and binding should match the descriptor set layout
struct PointLight
{
    mat4 lightMVP; // matrix for light
    vec4 position; // ignore w
    vec4 color;    // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUBO
{
    uint pickedObjectID;
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor; // w is intensity
    PointLight pointLight;
} ubo;

layout(push_constant) uniform Pushdata
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} pushdata;

layout(set = 0, binding = 1) buffer ShaderStorageBufferObject
{
    float Selected_ID;
} ssbo;


void main()
{
    // return pick ID back to CPU through SSBO
    ssbo.Selected_ID = inID;
    outColor = inID; // for debugging purpose
}

