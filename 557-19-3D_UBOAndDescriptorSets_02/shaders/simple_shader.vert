#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;

const float AMBIENT = 0.02; // to simulate indirect illumination - directionless ambient light (try without it)

// Set and binding should match the descriptor set layout
layout(set = 0, binding = 0) uniform GlobalUBO
{
    mat4 projectionViewMatrix;
    vec3 directionToLight;
} ubo;

// Note 128 bytes can only contain 2 4x4 matrices, so we have run into the limitation
layout(push_constant) uniform Pushdata
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} pushdata;


void main()
{
    gl_Position = ubo.projectionViewMatrix * pushdata.modelMatrix * vec4(position, 1.0);
  
    // Note: this ony works in certain condition
    // it is only correct if uniform scaling is applied (sz == sy == sz)
    vec3 normalWorldSpace = normalize(mat3(pushdata.modelMatrix) * normal);

    // If normal is away from the light source, the dot product can be negative. So we need to set the lower bound to 0
    float lightIntensity = AMBIENT + max(dot(normalWorldSpace, ubo.directionToLight), 0);

    fragColor = lightIntensity * color;
}

