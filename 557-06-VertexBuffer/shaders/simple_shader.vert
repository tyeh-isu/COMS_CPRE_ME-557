#version 450

// The viewport is from (-1,-1) - top left to (1,1) - bottom right
// Center is (0,0)
layout(location = 0) in vec2 position;

layout(push_constant) uniform Pushdata
{
    float time;
} pushdata;


void main()
{
    gl_Position = vec4(position.x + pushdata.time, position.y, 0.0, 1.0);
}

