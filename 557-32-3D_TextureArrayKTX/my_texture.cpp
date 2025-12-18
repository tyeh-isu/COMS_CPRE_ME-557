#include "my_texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// std
#include <cstring>
#include <stdexcept>
#include <cmath>
#include <fstream>
#include <assert.h>

#define MAX_LAYERS 8

MyTexture::MyTexture(MyDevice& device, std::string textureFileName) :
    m_myDevice{ device },
    m_vkTexureFormat{ VK_FORMAT_R8G8B8A8_SRGB },
    m_iMipLevels{ 1 },
    m_iTexLayers{ 0 }
{
    // Support ktx file format
    if (textureFileName.substr(textureFileName.find_last_of(".") + 1) == "ktx")
        _createTextureImage_ktx(textureFileName);
    else
        _createTextureImage(textureFileName);

    _createTextureImageView();
    _createTextureSampler();
}

MyTexture::~MyTexture()
{
    // Clean up
    vkDestroySampler(m_myDevice.device(), m_vkTextureSampler, nullptr);
    vkDestroyImageView(m_myDevice.device(), m_vkTextureImageView, nullptr);

    vkDestroyImage(m_myDevice.device(), m_vkTextureImage, nullptr);
    vkFreeMemory(m_myDevice.device(), m_vkTextureImageMemory, nullptr);
}

void MyTexture::_createTextureImage_ktx(std::string textureFileName)
{
    ktxResult result;
    ktxTexture* ktxTexture;

    result = _loadKTXFile(textureFileName, &ktxTexture);

    if (result != KTX_SUCCESS)
    {
        throw std::runtime_error("failed to load KTX texture image!");
    }

    uint32_t texWidth, texHeight;
    texWidth = ktxTexture->baseWidth;
    texHeight = ktxTexture->baseHeight;
    m_iMipLevels = ktxTexture->numLevels;
    m_iTexLayers = ktxTexture->numLayers;
    assert(m_iTexLayers <= MAX_LAYERS);

    // Keep it simple for now that only produce mipmap for the first layer
    if (m_iTexLayers > 0)
        m_iMipLevels = 1;

    ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
    ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);
    m_vkTexureFormat = VK_FORMAT_R8G8B8A8_UNORM;

    // Use staging buffer to copy texture into GPU
    // Create a host-visible staging buffer that contains the raw image data
    // This buffer will be the data source for copying texture data to the optimal tiled image on the device
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    m_myDevice.createBuffer(ktxTextureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    uint8_t* data;
    vkMapMemory(m_myDevice.device(), stagingBufferMemory, 0, ktxTextureSize, 0, (void**)&data);
    memcpy(data, ktxTextureData, static_cast<size_t>(ktxTextureSize));
    vkUnmapMemory(m_myDevice.device(), stagingBufferMemory);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = texWidth;
    imageInfo.extent.height = texHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = m_iMipLevels;
    imageInfo.arrayLayers = m_iTexLayers;
    imageInfo.format = m_vkTexureFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    m_myDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vkTextureImage, m_vkTextureImageMemory);

    // Setup buffer copy regions for each mip level
    std::vector<VkBufferImageCopy> bufferCopyRegions;
    uint32_t offset = 0;

    // To keep this simple, we will only load layers and no mip level
    if (m_iTexLayers > 0)
    {
        for (uint32_t layer = 0; layer < m_iTexLayers; layer++)
        {
            // Calculate offset into staging buffer for the current array layer
            ktx_size_t offset;
            KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, 0, layer, 0, &offset);
            assert(ret == KTX_SUCCESS);
            // Setup a buffer image copy structure for the current array layer
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth;
            bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offset;
            bufferCopyRegions.push_back(bufferCopyRegion);
        }
    }
    else
    {
        for (uint32_t i = 0; i < m_iMipLevels; i++)
        {
            // Calculate offset into staging buffer for the current mip level
            ktx_size_t offset;
            KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
            assert(ret == KTX_SUCCESS);
            // Setup a buffer image copy structure for the current mip level
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = i;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> i;
            bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> i;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offset;
            bufferCopyRegions.push_back(bufferCopyRegion);
        }
    }

    // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
    _transitionImageLayout(m_vkTextureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_iMipLevels, m_iTexLayers);

    // Copy the images from the staging buffer to the optimal tiled image
    m_myDevice.copyBufferRigonToImage(stagingBuffer, m_vkTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), bufferCopyRegions);

    // Change texture image layout to shader read after all faces have been copied
    _transitionImageLayout(m_vkTextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_iMipLevels, m_iTexLayers);

    vkDestroyBuffer(m_myDevice.device(), stagingBuffer, nullptr);
    vkFreeMemory(m_myDevice.device(), stagingBufferMemory, nullptr);

    // Delete the memory at the end
    ktxTexture_Destroy(ktxTexture);
}

void MyTexture::_createTextureImage(std::string textureFileName)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(textureFileName.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = VkDeviceSize(texWidth * texHeight * 4);

    m_vkTexureFormat = VK_FORMAT_R8G8B8A8_SRGB;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    m_myDevice.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_myDevice.device(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_myDevice.device(), stagingBufferMemory);

    stbi_image_free(pixels);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = texWidth;
    imageInfo.extent.height = texHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = m_iMipLevels;
    imageInfo.arrayLayers = m_iTexLayers;
    imageInfo.format = m_vkTexureFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    m_myDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vkTextureImage, m_vkTextureImageMemory);

    _transitionImageLayout(m_vkTextureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_iMipLevels, m_iTexLayers);
    m_myDevice.copyBufferToImage(stagingBuffer, m_vkTextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1);
    _transitionImageLayout(m_vkTextureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_iMipLevels, m_iTexLayers);

    vkDestroyBuffer(m_myDevice.device(), stagingBuffer, nullptr);
    vkFreeMemory(m_myDevice.device(), stagingBufferMemory, nullptr);
}

// Note: there is a similar function in MySwapChain to create color and depth image views for each image target in the swap chain
void MyTexture::_createTextureImageView()
{
    if (m_vkTextureImage == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to call ");
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_vkTextureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format = m_vkTexureFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_iMipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = m_iTexLayers;

    if (vkCreateImageView(m_myDevice.device(), &viewInfo, nullptr, &m_vkTextureImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }
}

void MyTexture::_createTextureSampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_myDevice.physicalDevice(), &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = (float)m_iMipLevels;

    if (vkCreateSampler(m_myDevice.device(), &samplerInfo, nullptr, &m_vkTextureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void MyTexture::_transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t texLayers)
{
    VkCommandBuffer commandBuffer = m_myDevice.beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = texLayers;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    m_myDevice.endSingleTimeCommands(commandBuffer);
}

VkDescriptorImageInfo MyTexture::descriptorInfo()
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_vkTextureImageView;
    imageInfo.sampler = m_vkTextureSampler;

    return imageInfo;
}

ktxResult MyTexture::_loadKTXFile(std::string filename, ktxTexture** target)
{
    ktxResult result = KTX_SUCCESS;

    std::ifstream fs(filename.c_str());

    if (fs.fail())
    {
        throw std::runtime_error("Could not load texture from " + filename + "\n\nMake sure the assets submodule has been checked out and is up-to-date.");
    }

    result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, target);

    return result;
}

