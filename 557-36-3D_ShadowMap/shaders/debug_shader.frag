#version 450

// Only location and type (vec3) matter, the name doesn't need to match

#define RENDER_SHADOW 0

#if RENDER_SHADOW
layout (set = 0, binding = 2) uniform sampler2D samplerColor[3];
#else
layout (set = 0, binding = 2) uniform sampler2D samplerColor[2];
#endif

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

struct PointLight
{
    mat4 lightMVP; // matrix for light
    vec4 position; // ignore w
    vec4 color;    // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUBO
{
    uint pickedObjectID;
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor; // w is intensity
    PointLight pointLight;
} ubo;


layout(push_constant) uniform Pushdata
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} pushdata;

float LinearizeDepth(float depth)
{
  float n = 0.1;
  float f = 96.0;
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));	
}

const mat2 rotateY = mat2(
    0.0, -1.0,
    1.0, 0.0
);

void main()
{
    vec2 rotuv = rotateY * inUV;

#if RENDER_SHADOW
    float depth = texture(samplerColor[2], rotuv).r;
#else
    float depth = texture(samplerColor[1], rotuv).r;
#endif

    //depth = clamp(1.0 - depth, 0, 1);
    //outColor = vec4(depth, depth, depth, 1.0);

	outColor = vec4(vec3(LinearizeDepth(depth)), 1.0);

    //outColor = vec4(1.0, 0.0, 0.0, 1.0);
}

