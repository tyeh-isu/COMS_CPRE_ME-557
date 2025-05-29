#version 450

// The viewport is from (-1,-1) - top left to (1,1) - bottom right
// Center is (0,0)

layout(push_constant) uniform Pushdata
{
    float time;
} pushdata;

vec2 positions[1] = vec2[](
    vec2(0.0, 0.0)
);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex].x + pushdata.time, positions[gl_VertexIndex].y, 0.0, 1.0);
    gl_PointSize = 10;
}
