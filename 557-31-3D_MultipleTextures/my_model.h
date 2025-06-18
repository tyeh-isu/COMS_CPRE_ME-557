#ifndef __MY_MODEL_H__
#define __MY_MODEL_H__

#include "my_buffer.h"
#include "my_device.h"

// use radian rather degree for angle
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// Std
#include <memory>
#include <vector>

// Read vertex data from CPU and copy the data into GPU
class MyModel
{
public:

	struct Vertex
	{
		glm::vec3 position{};
		glm::vec3 color{};
		glm::vec3 normal{};
		glm::vec2 uv{};
		float     id;

		bool operator==(const Vertex& other) const
		{
			return (position == other.position && color == other.color && normal == other.normal &&
				uv == other.uv);
		}
	};

	struct Builder
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		void loadModel(const std::string& filepath, unsigned int id);
	};

	static std::vector<VkVertexInputBindingDescription>   getBindingDescriptions();
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

	MyModel(MyDevice &device, const std::vector<Vertex>& vertices);
	MyModel(MyDevice& device, const MyModel::Builder& builder);

	// note: id is used for picking
	// if id == 0.0f; no picking
	static std::unique_ptr<MyModel> createModelFromFile(
		MyDevice& device, const std::string& filepath, unsigned int id = 0);

	void bind(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer);

private:

	void _createVertexBuffer(const std::vector<Vertex>& vertices, bool bUseIndexBuffer = false);
	void _createIndexBuffers(const std::vector<uint32_t>& indices);

	// Use the new MyBuffer for vertex and index buffers
	MyDevice&                 m_pMyDevice;
	std::unique_ptr<MyBuffer> m_pMyVertexBuffer;
	uint32_t                  m_iVertexCount = 0;

	bool                      m_bHasIndexBuffer = false;
	std::unique_ptr<MyBuffer> m_pMyIndexBuffer;
	uint32_t                  m_iIndexCount = 0;
};

#endif

