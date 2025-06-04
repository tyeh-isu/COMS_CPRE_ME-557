#version 450

layout (location = 0) in vec2 fragOffset;
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

void main()
{
    // calculate the radius of this fragment
    float dis = sqrt(dot(fragOffset, fragOffset));
    if (dis >= 1.0)
    {
        discard; // if it is outside, don't draw
    }
    outColor = ubo.pointLight.color;
}

