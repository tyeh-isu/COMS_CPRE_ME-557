#include "my_model.h"
#include <cassert>

MyModel::MyModel(MyDevice& device, const std::vector<Vertex>& vertices) :
	m_myDevice{ device },
	m_iVertexCount{ 0 }
{
	_createVertexBuffer(vertices, false);
}

MyModel::MyModel(MyDevice& device, const MyModel::Builder& builder) : 
	m_myDevice{ device },
	m_iVertexCount{ 0 }
{
	_createVertexBuffer(builder.vertices, true);
	_createIndexBuffers(builder.indices);
}

MyModel::~MyModel()
{
    vkDestroyBuffer(m_myDevice.device(), m_vkVertexBuffer, nullptr);
    vkFreeMemory(m_myDevice.device(), m_vkVertexBufferMemory, nullptr);

	if (m_bHasIndexBuffer)
	{
		vkDestroyBuffer(m_myDevice.device(), m_vkIndexBuffer, nullptr);
		vkFreeMemory(m_myDevice.device(), m_vkIndexBufferMemory, nullptr);
	}
}

void MyModel::_createVertexBuffer(const std::vector<Vertex>& vertices, bool bUseIndexBuffer)
{
    m_iVertexCount = static_cast<uint32_t>(vertices.size());
    assert(m_iVertexCount >= 3 && "Vertext count must be at least 3");
    
    // number of bytes need to store the vertex buffer
    // Note: we assume Color and Position are interleaved here
    // inside the vertex buffer
    VkDeviceSize bufferSize = sizeof(vertices[0]) * m_iVertexCount;
    
    // Create buffer handle and allocate buffer memory on GPU side
    // Note: Host - CPU
    //       Device - GPU
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT - make sure the GPU memory is visible to the host
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT - make sure the host and device memory is consistent

    // Note: Use stage buffer to copy memory buffer can be faster
	// because VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT can be slower
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	if (bUseIndexBuffer)
	{
		m_myDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data = nullptr;
		vkMapMemory(m_myDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(m_myDevice.device(), stagingBufferMemory);

		m_myDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			m_vkVertexBuffer,
			m_vkVertexBufferMemory);

		m_myDevice.copyBuffer(stagingBuffer, m_vkVertexBuffer, bufferSize);

		vkDestroyBuffer(m_myDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(m_myDevice.device(), stagingBufferMemory, nullptr);
	}
	else
	{
		m_myDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			m_vkVertexBuffer,
			m_vkVertexBufferMemory);

		// Because of VK_MEMORY_PROPERTY_HOST_COHERENT_BIT is set
		// the memory of data on CPU side will copy to the memory to GPU automatically
		void* data;
		vkMapMemory(m_myDevice.device(), m_vkVertexBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(m_myDevice.device(), m_vkVertexBufferMemory);
	}
}

void MyModel::_createIndexBuffers(const std::vector<uint32_t>& indices) 
{
	m_iIndexCount = static_cast<uint32_t>(indices.size());
	m_bHasIndexBuffer = m_iIndexCount > 0;

	if (!m_bHasIndexBuffer) 
	{
		return;
	}

	VkDeviceSize bufferSize = sizeof(indices[0]) * m_iIndexCount;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	m_myDevice.createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	// Map the CPU data block to GPU data block
	// when memcpy is called, it will also copy the memory to GPU
	void* data;
	vkMapMemory(m_myDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(m_myDevice.device(), stagingBufferMemory);

	m_myDevice.createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vkIndexBuffer,
		m_vkIndexBufferMemory);

	m_myDevice.copyBuffer(stagingBuffer, m_vkIndexBuffer, bufferSize);

	vkDestroyBuffer(m_myDevice.device(), stagingBuffer, nullptr);
	vkFreeMemory(m_myDevice.device(), stagingBufferMemory, nullptr);
}

void MyModel::bind(VkCommandBuffer commandBuffer)
{
    VkBuffer buffers[] = { m_vkVertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

	if (m_bHasIndexBuffer) 
	{
		vkCmdBindIndexBuffer(commandBuffer, m_vkIndexBuffer, 0, VK_INDEX_TYPE_UINT32); // can store up to 2^32 -1 indices
	}
}

void MyModel::draw(VkCommandBuffer commandBuffer)
{
	if (m_bHasIndexBuffer)
	{
		vkCmdDrawIndexed(commandBuffer, m_iIndexCount, 1, 0, 0, 0);
	}
	else
	{
		vkCmdDraw(commandBuffer, m_iVertexCount, 1, 0, 0);
	}
}

std::vector<VkVertexInputBindingDescription> MyModel::getBindingDescriptions()
{
    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  
    return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> MyModel::getAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2); // now we have vertex and color
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
    attributeDescriptions[0].offset = offsetof(Vertex, position); // same as 0, but just more clear
    
    attributeDescriptions[1].binding = 0;  // interleave both vertex and color into the same array
    attributeDescriptions[1].location = 1; // location 1 is now color
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
    attributeDescriptions[1].offset = offsetof(Vertex, color);    // calcualte the byte offset in the Vertex struct
    
    return attributeDescriptions;
    
    // note : this function can be simplified as the following
    // return {
    //           {0, 0, VK_FORMAT_R32G32_SFLOAT,   offsetof(Vertex, position)},
    //           {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)}
    //        };
}

