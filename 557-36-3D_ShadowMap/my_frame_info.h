#ifndef __MY_FRAMEINFO_H__
#define __MY_FRAMEINFO_H__

#include "my_camera.h"
#include "my_game_object.h"

// lib
#include <vulkan/vulkan.h>

struct MyPointLight 
{
	glm::mat4 lightMVP;
	glm::vec4 position{ -1.2f, 1.5f, 0.0f, 1.0f};     // ignore w
	glm::vec4 color{1.0f, 1.0f, 0.6f, 1.0f};          // w is intensity
};

struct MyGlobalUBO 
{
	alignas(4)  unsigned int pickedObjectID = 0; // put it first for better alignment
    alignas(16) glm::mat4 projection{ 1.0f };
    alignas(16) glm::mat4 view{ 1.0f };
    alignas(16) glm::mat4 inverseView{ 1.0f };
    alignas(16) glm::vec4 ambientLightColor{ 1.0f, 1.0f, 1.0f, 0.01f }; // w is intensity
    MyPointLight pointLight;
};

struct MySSBO
{
	float id;
};

struct MySimplePushConstantData
{
	glm::mat4 modelMatrix{ 1.0f };
	glm::mat4 normalMatrix{ 1.0f };
};

struct MyFrameInfo
{
	int                frameIndex;
	float              frameTime;
	VkCommandBuffer    commandBuffer;
	MyCamera&          camera;
	VkDescriptorSet    globalDescriptorSet;
	VkDescriptorSet    offscreenDescriptorSet;
	MyGameObject::Map& gameObjects;
};

#endif

