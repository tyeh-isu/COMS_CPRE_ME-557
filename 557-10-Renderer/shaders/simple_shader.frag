#version 450

// Only location and type (vec4) matter, the name doesn't need to match

layout(location = 0) in vec4 inColor;
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
    outColor = inColor;
    //outColor = vec4(pushdata.push_color, 1.0);
}

