#version 450
#extension GL_EXT_shader_atomic_float2 : enable // enable this extenstion in order to use atomicMin

/* only location and type (vec3) matter, the name doesn't need to match */

layout(location = 0) flat in float inID; // flat means no interpolation
layout(location = 0) out float outColor;

// Set and binding should match the descriptor set layout
struct PointLight
{
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
    float depth;
} ssbo;


void main()
{
    // return pick ID back to CPU through SSBO

    // use atomicMin to find the fragment closet to the viewpoint, similar to depth test
    float currentDepth = gl_FragCoord.z;
    atomicMin(ssbo.depth, currentDepth);

    // Set the ID to SSBO for the fragment closet to the viewpoint
    if (currentDepth == ssbo.depth)
    {
       ssbo.Selected_ID = inID;
    }

    outColor = inID; // for debugging purpose
}

