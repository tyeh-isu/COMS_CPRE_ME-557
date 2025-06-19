#ifndef __MY_TEXTURE_H__
#define __MY_TEXTURE_H__

#include "my_device.h"
#include "ktx/include/ktx.h"

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

	unsigned int numTextureLayers() { return (unsigned int)m_iTexLayers; }

private:
	void _createTextureImage(std::string textureFileName);
	void _createTextureImage_ktx(std::string textureFileName);
	void _createTextureImageView();
	void _createTextureSampler();
	void _transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t texLayers);

	std::string    m_sTextureFile;
	MyDevice&      m_myDevice;

	VkImage        m_vkTextureImage = VK_NULL_HANDLE;
	VkDeviceMemory m_vkTextureImageMemory;
	VkImageView    m_vkTextureImageView;
	VkSampler      m_vkTextureSampler;
	VkFormat       m_vkTexureFormat;

	uint32_t       m_iMipLevels;
	uint32_t       m_iTexLayers;

	// Ktx
	ktxResult _loadKTXFile(std::string filename, ktxTexture** target);
};

#endif

