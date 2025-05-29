#version 450

// The viewport is from (-1,-1) - top left to (1,1) - bottom right
// Center is (0,0)
vec2 positions[1] = vec2[](
    vec2(-1.0, -1.0)
);

//vec2 positions[3] = vec2[](
//  vec2(0.0, -0.5),
//  vec2(0.5, 0.5),
//  vec2(-0.5, 0.5)
//);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    gl_PointSize = 10;
}

