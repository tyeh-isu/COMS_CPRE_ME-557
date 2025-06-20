#version 450

layout (location = 0) in vec2 fragOffset;
layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUBO
{
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor; // w is intensity
    vec3 lightPosition;
    vec4 lightColor;
} ubo;


void main()
{
    // calculate the radius of this fragment
    float dis = sqrt(dot(fragOffset, fragOffset));
    if (dis >= 1.0)
    {
        discard; // if it is outside, don't draw
    }
    outColor = vec4(ubo.lightColor.xyz, 1.0);
}

