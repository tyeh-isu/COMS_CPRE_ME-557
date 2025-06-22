#ifndef __MY_FRAMEINFO_H__
#define __MY_FRAMEINFO_H__

#include "my_camera.h"
#include "my_game_object.h"

// lib
#include <vulkan/vulkan.h>

struct MyPointLight 
{
	glm::vec4 position{ -0.7f, 1.0f, 1.0, 1.0f};      // ignore w
	glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};          // w is intensity
};

struct MyGlobalUBO 
{
    alignas(16) glm::mat4 projection{ 1.0f };
    alignas(16) glm::mat4 view{ 1.0f };
    alignas(16) glm::mat4 inverseView{ 1.0f };
    alignas(16) glm::vec4 ambientLightColor{ 1.0f, 1.0f, 1.0f, 0.01f }; // w is intensity
    MyPointLight pointLight;
};

struct MyFrameInfo
{
	int                frameIndex;
	float              frameTime;
	VkCommandBuffer    commandBuffer;
	MyCamera&          camera;
	VkDescriptorSet    globalDescriptorSet;
	MyGameObject::Map& gameObjects;
};

#endif

