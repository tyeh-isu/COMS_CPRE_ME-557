#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;
layout(location = 4) in float inID; // for picking. not used for simple rendering

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) out vec4 outShadowCoord;
layout(location = 5) out float selected;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

const mat4 rotateY = mat4(
    -1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, -1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
);

struct PointLight
{
    mat4 lightMVP; // matrix for light
    vec4 position; // ignore w
    vec4 color;    // w is intensity
};

// Set and binding should match the descriptor set layout
layout(set = 0, binding = 0) uniform GlobalUBO
{
    uint pickedObjectID;
    mat4 projection;
    mat4 view;
    mat4 invView;
    vec4 ambientLightColor; // w is intensity
    PointLight pointLight;
} ubo;

// Note 128 bytes can only contain 2 4x4 matrices, so we have run into the limitation
// Please note that this application requires more than 128 bytes for push constant
// it will fail to start if the device cannot support above 128 bytes push contant
layout(push_constant) uniform Pushdata
{
    uint textureID;
    mat4 modelMatrix;
    mat4 normalMatrix;
} pushdata;


void main()
{
    vec4 positionWorld = pushdata.modelMatrix * vec4(position, 1.0);
    gl_Position = ubo.projection * ubo.view * positionWorld;

    fragNormalWorld = normalize(mat3(pushdata.normalMatrix) * normal);
    fragPosWorld = positionWorld.xyz;

    if (ubo.pickedObjectID == uint(inID)) // if select the part, render as selection color (red)
    {
        fragColor = vec3(1.0, 0.0, 0.0); // fragColor is not used in frag shader
        selected = 1.0f;
    }
    else
    {
        fragColor = color; // fragColor is not used in frag shader
        selected = 0.0f;
    }

    fragTexCoord = uv;

    // Shadow normalized coordinates
    outShadowCoord =  biasMat * ubo.pointLight.lightMVP * positionWorld;
}

