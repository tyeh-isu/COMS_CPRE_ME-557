#version 450

#define RENDER_SHADOW 1

#if RENDER_SHADOW
layout (set = 0, binding = 2) uniform sampler2D texSampler[3];
#else
layout (set = 0, binding = 2) uniform sampler2D texSampler[2];
#endif

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec3 fragPosWorld;
layout (location = 2) in vec3 fragNormalWorld;
layout (location = 3) in vec2 fragTexCoord;
layout (location = 4) in vec4 inShadowCoord;
layout (location = 5) in float selected;

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
    uint textureID;
    mat4 modelMatrix;
    mat4 normalMatrix;
} pushdata;

// SSBO is not used in regular rendering
// layout(set = 0, binding = 1) buffer ShaderStorageBufferObject
//{
//    float Selected_ID;
//} ssbo;

#define ambient 0.1

#if RENDER_SHADOW
float textureProj(vec4 shadowCoord, vec2 off)
{
    vec2 v_texCoord;
    v_texCoord = shadowCoord.st;

    float shadow = 1.0;

    // If the shadow image is outside of view frustum of the light, don't show shadow.
    // Please note that it may cause some rendering artifect
    if ( v_texCoord.x > 0.9999 || v_texCoord.x < 0.0001 || v_texCoord.y > 0.9999 || v_texCoord.y < 0.0001 )
        return shadow;

	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( texSampler[2], v_texCoord + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = ambient;
		}
	}

	return shadow;
}

float filterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(texSampler[2], 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
			count++;
		}
	
	}
	return shadowFactor / count;
}
#endif


void main()
{
#if RENDER_SHADOW
    float shadow = textureProj(inShadowCoord / inShadowCoord.w, vec2(0,0));
#else
    float shadow = 1.0f;
#endif

    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(fragNormalWorld);

    vec3 cameraPosWorld = ubo.invView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);
    int index = int(pushdata.textureID);

    vec4 texColor = texture(texSampler[index], fragTexCoord);

    // If selected, turn into red color
    if (selected != 0.0f)
      texColor = vec4(1.0, 0.0, 0.0, 1.0);

    vec3 s_texColor = texColor.xyz * shadow;

    PointLight light = ubo.pointLight;
    vec3 directionToLight = light.position.xyz - fragPosWorld;
    float attenuation = 1.0 / dot(directionToLight, directionToLight); // distance squared
    directionToLight = normalize(directionToLight);

    // diffuse lighting
    float cosAngIncidence = max(dot(surfaceNormal, directionToLight), 0);
    vec3 intensity = light.color.xyz * light.color.w * attenuation;
    diffuseLight += intensity * cosAngIncidence;

    // specular lighting
    vec3 halfAngle = normalize(directionToLight + viewDirection);
    float blinnTerm = dot(surfaceNormal, halfAngle);
    blinnTerm = clamp(blinnTerm, 0, 1);
    blinnTerm = pow(blinnTerm, 32.0); // higher values -> sharper highlight
    specularLight += intensity * blinnTerm;

    outColor = vec4(diffuseLight * s_texColor + specularLight * s_texColor, 1.0);
}

