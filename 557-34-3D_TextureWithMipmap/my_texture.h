#ifndef __MY_TEXTURE_H__
#define __MY_TEXTURE_H__

#include "my_device.h"

class MyTexture
{
public:
	MyTexture(MyDevice& device, std::string textureFileName);
	~MyTexture();

	MyTexture(const MyTexture&) = delete;
	MyTexture& operator=(const MyTexture&) = delete;
	MyTexture(MyTexture&&) = delete;
	MyTexture& operator=(const MyTexture&&) = delete;
	VkDescriptorImageInfo descriptorInfo();

private:
	void _createTextureImage(std::string textureFileName);
	void _createTextureImageView();
	void _createTextureSampler();
	void _transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	void _generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	std::string    m_sTextureFile;
	MyDevice&      m_myDevice;
	uint32_t       m_iMipLevels;

	// TODO: need to move these into SwapChain class
	VkImage        m_vkTextureImage = VK_NULL_HANDLE;
	VkDeviceMemory m_vkTextureImageMemory;
	VkImageView    m_vkTextureImageView;
	VkSampler      m_vkTextureSampler;
};

#endif

