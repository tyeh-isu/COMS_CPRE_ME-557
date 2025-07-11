#version 450

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Pushdata
{
    mat2 transform;
    vec2 offset;
    vec3 push_color;
} pushdata;


void main()
{
    // r g b a
    outColor = vec4(pushdata.push_color, 1.0);
}

