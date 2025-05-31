#ifndef __MY_MODEL_H__
#define __MY_MODEL_H__

#include "my_device.h"

// use radian rather degree for angle
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// Std
#include <vector>

// Read vertex data from CPU and copy the data into GPU
class MyModel
{
public:

	struct Vertex
	{
		glm::vec3 position{};
		glm::vec3 color{};
	};

	struct Builder
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
	};

	static std::vector<VkVertexInputBindingDescription>   getBindingDescriptions();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

	MyModel(MyDevice &device, const std::vector<Vertex>& vertices);
	MyModel(MyDevice& device, const MyModel::Builder& builder);

	~MyModel();

	MyModel(const MyModel&) = delete;
	MyModel& operator=(const MyModel&) = delete;
	MyModel(MyModel&&) = delete;
	MyModel& operator=(const MyModel&&) = delete;

	void bind(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer);

private:

	void _createVertexBuffer(const std::vector<Vertex>& vertices, bool bUseIndexBuffer = false);
	void _createIndexBuffers(const std::vector<uint32_t>& indices);

	MyDevice&      m_myDevice;
	VkBuffer       m_vkVertexBuffer;       // handle of the buffer on GPU side
	VkDeviceMemory m_vkVertexBufferMemory; // memory on GPU side of the buffer
	uint32_t       m_iVertexCount;

	bool           m_bHasIndexBuffer = false;
	VkBuffer       m_vkIndexBuffer;
	VkDeviceMemory m_vkIndexBufferMemory;
	uint32_t       m_iIndexCount;
};

#endif

