#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;
layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform Pushdata
{
    mat2 transform;
    vec2 offset;
    vec3 push_color;
} pushdata;

void main()
{
    gl_Position = vec4(pushdata.transform * position + pushdata.offset, 0.0, 1.0);
    outColor = vec4(color, 1.0);
}

