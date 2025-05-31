#version 450

// Only location and type (vec3) matter. The name doesn't need to match
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;


void main()
{
    // r g b a
    outColor = vec4(fragColor, 1.0);
}

