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
	void _transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	std::string    m_sTextureFile;
	MyDevice&      m_myDevice;

	VkImage        m_vkTextureImage = VK_NULL_HANDLE;
	VkDeviceMemory m_vkTextureImageMemory;
	VkImageView    m_vkTextureImageView;
	VkSampler      m_vkTextureSampler;
};

#endif

