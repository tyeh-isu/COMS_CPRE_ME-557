#include "my_swap_chain.h"

// std
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

MySwapChain::MySwapChain(MyDevice &deviceRef, VkExtent2D extent)
    : m_myDevice{deviceRef}, 
      m_vkWindowExtent{extent},
      m_iCurrentFrame(0)
{
    _init();
}

MySwapChain::MySwapChain(MyDevice& deviceRef, VkExtent2D extent, std::shared_ptr<MySwapChain> previous)
    : m_myDevice{ deviceRef }, 
      m_vkWindowExtent{ extent },
      m_pMyOldSwapChain{ previous },
      m_iCurrentFrame(0)
{
    _init();

    // Clean up old swap chain since it is no longer needed
    m_pMyOldSwapChain = nullptr;
}

void MySwapChain::_init()
{
    _createSwapChainResources();
    _createRenderPass();
    _createDepthResources();
    _createFramebuffers();
    _createSyncObjects();

    // For picking
    _createPickingResources();
}

MySwapChain::~MySwapChain() 
{
    for (auto imageView : m_vVkSwapChainImageViews)
    {
        vkDestroyImageView(m_myDevice.device(), imageView, nullptr);
    }
    m_vVkSwapChainImageViews.clear();

    if (m_vkSwapChain != nullptr)
    {
        vkDestroySwapchainKHR(m_myDevice.device(), m_vkSwapChain, nullptr);
        m_vkSwapChain = nullptr;
    }

    for (int i = 0; i < m_vVkDepthImages.size(); i++)
    {
        vkDestroyImageView(m_myDevice.device(), m_vVkDepthImageViews[i], nullptr);
        vkDestroyImage(m_myDevice.device(), m_vVkDepthImages[i], nullptr);
        vkFreeMemory(m_myDevice.device(), m_vVkDepthImageMemorys[i], nullptr);
    }

    for (auto framebuffer : m_vVkSwapChainFramebuffers)
    {
        vkDestroyFramebuffer(m_myDevice.device(), framebuffer, nullptr);
    }

    vkDestroyRenderPass(m_myDevice.device(), m_vkRenderPass, nullptr);

    // Picking
    vkDestroyImageView(m_myDevice.device(), m_vkPickColorImageView, nullptr);
    vkDestroyImageView(m_myDevice.device(), m_vkPickDepthImageView, nullptr);
    vkDestroyImage(m_myDevice.device(), m_vkPickColorImage, nullptr);
    vkDestroyImage(m_myDevice.device(), m_vkPickDepthImage, nullptr);
    vkFreeMemory(m_myDevice.device(), m_vkPickColorImageMemory, nullptr);
    vkFreeMemory(m_myDevice.device(), m_vkPickDepthImageMemory, nullptr);
    vkDestroyFramebuffer(m_myDevice.device(), m_vkPickFrameBuffer, nullptr);
    vkDestroyRenderPass(m_myDevice.device(), m_vkPickRenderPass, nullptr);

    // cleanup synchronization objects
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(m_myDevice.device(), m_vVkImageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_myDevice.device(), m_vVkRenderFinishedSemaphores[i], nullptr);
        vkDestroyFence(m_myDevice.device(), m_vVkInFlightFences[i], nullptr);
    }
}

VkResult MySwapChain::acquireNextImage(uint32_t *imageIndex)
{
    // Follow this pattern
    // https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html

    // Note: CPU will wait here at fences
    vkWaitForFences(
        m_myDevice.device(),
        1,
        &m_vVkInFlightFences[m_iCurrentFrame],
        VK_TRUE,
        std::numeric_limits<uint64_t>::max());

    // When CPU is free, reset the fence
    vkResetFences(m_myDevice.device(), 1, &m_vVkInFlightFences[m_iCurrentFrame]);

    VkResult result = vkAcquireNextImageKHR(
        m_myDevice.device(),
        m_vkSwapChain,
        std::numeric_limits<uint64_t>::max(),
        m_vVkImageAvailableSemaphores[m_iCurrentFrame],  // must be a not signaled semaphore
        VK_NULL_HANDLE,
        imageIndex);

    return result;
}

VkResult MySwapChain::submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex)
{
    // Follow this pattern
    // https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Please note that m_iCurrentFrame may not be the same as *imageIndex that passed in from MyRenderer
    // m_iCurrentFrame identifies the current in-flight frame (meaning the frame that has been submitted to GPU but hasn't yet finished)
    // *imageIndex identifies the acquired frame from the swap chain to be submitted to GPU
    VkSemaphore acquire_semaphore [] = {m_vVkImageAvailableSemaphores[m_iCurrentFrame]};
    VkSemaphore submit_semaphore [] = {m_vVkRenderFinishedSemaphores[*imageIndex]};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = acquire_semaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = submit_semaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;

    if (vkQueueSubmit(m_myDevice.graphicsQueue(), 1, &submitInfo, m_vVkInFlightFences[m_iCurrentFrame]) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = submit_semaphore;

    VkSwapchainKHR swapChains[] = { m_vkSwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = imageIndex;

    auto result = vkQueuePresentKHR(m_myDevice.presentQueue(), &presentInfo);

    // Update frame index
    m_iCurrentFrame = (m_iCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}

void MySwapChain::_createSwapChainResources()
{
    SwapChainSupportDetails swapChainSupport = m_myDevice.getSwapChainSupport();

    VkSurfaceFormatKHR surfaceFormat = _chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = _chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = _chooseSwapExtent(swapChainSupport.capabilities);
 
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_myDevice.surface();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = m_myDevice.findPhysicalQueueFamilies();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } 
    else 
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;      // Optional
        createInfo.pQueueFamilyIndices = nullptr;  // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = m_pMyOldSwapChain == nullptr ? VK_NULL_HANDLE : m_pMyOldSwapChain->m_vkSwapChain;

    if (vkCreateSwapchainKHR(m_myDevice.device(), &createInfo, nullptr, &m_vkSwapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    // we only specified a minimum number of images in the swap chain, so the implementation is
    // allowed to create a swap chain with more. That's why we'll first query the final number of
    // images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
    // retrieve the handles.
    vkGetSwapchainImagesKHR(m_myDevice.device(), m_vkSwapChain, &imageCount, nullptr);
    m_vVkSwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_myDevice.device(), m_vkSwapChain, &imageCount, m_vVkSwapChainImages.data());

    m_vkSwapChainImageFormat = surfaceFormat.format;
    m_vkSwapChainExtent = extent;

    // Create swap chain image view
    m_vVkSwapChainImageViews.resize(m_vVkSwapChainImages.size());
    for (size_t i = 0; i < m_vVkSwapChainImages.size(); i++)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_vVkSwapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_vkSwapChainImageFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_myDevice.device(), &viewInfo, nullptr, &m_vVkSwapChainImageViews[i]) != VK_SUCCESS)
		{
            throw std::runtime_error("failed to create texture image view!");
        }
    }
}

void MySwapChain::_createRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0; // index 0
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1; // index 1
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstSubpass = 0;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    if (vkCreateRenderPass(m_myDevice.device(), &renderPassInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void MySwapChain::_createFramebuffers() 
{
    m_vVkSwapChainFramebuffers.resize(imageCount());

    for (size_t i = 0; i < imageCount(); i++) 
    {
        std::array<VkImageView, 2> attachments = {m_vVkSwapChainImageViews[i], m_vVkDepthImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_vkRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_vkSwapChainExtent.width;
        framebufferInfo.height = m_vkSwapChainExtent.height;
        framebufferInfo.layers = 1;
        
        if (vkCreateFramebuffer(
            m_myDevice.device(),
            &framebufferInfo,
            nullptr,
            &m_vVkSwapChainFramebuffers[i]) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void MySwapChain::_createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();
    m_vkSwapChainDepthFormat = depthFormat;

    m_vVkDepthImages.resize(imageCount());
    m_vVkDepthImageMemorys.resize(imageCount());
    m_vVkDepthImageViews.resize(imageCount());

    for (int i = 0; i < m_vVkDepthImages.size(); i++)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = m_vkSwapChainExtent.width;
        imageInfo.extent.height = m_vkSwapChainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = 0;

        m_myDevice.createImageWithInfo(
            imageInfo,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_vVkDepthImages[i],
            m_vVkDepthImageMemorys[i]);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_vVkDepthImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_myDevice.device(), &viewInfo, nullptr, &m_vVkDepthImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture image view!");
        }
    }
}

void MySwapChain::_createSyncObjects()
{
    m_vVkImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_vVkRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_vVkInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
            // Semaphore used to ensure that image presentation is complete before starting to submit again
        if (vkCreateSemaphore(m_myDevice.device(), &semaphoreInfo, nullptr, &m_vVkImageAvailableSemaphores[i]) != VK_SUCCESS ||
            // Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
            vkCreateSemaphore(m_myDevice.device(), &semaphoreInfo, nullptr, &m_vVkRenderFinishedSemaphores[i]) != VK_SUCCESS ||
            // Fence used to ensure that command buffer has completed exection before using it again
            vkCreateFence(m_myDevice.device(), &fenceInfo, nullptr, &m_vVkInFlightFences[i]) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

VkSurfaceFormatKHR MySwapChain::_chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && // so the color can be correct. You can see the difference
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR MySwapChain::_chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    // NOte: the guarrantee present mode for all graphics card FIFO - to support V-sync
    //
    for (const auto &availablePresentMode : availablePresentModes)
	{
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
            std::cout << "Present mode: Mailbox" << std::endl;
            return availablePresentMode;
        }
    }

    // for (const auto &availablePresentMode : availablePresentModes) {
    //   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
    //     std::cout << "Present mode: Immediate" << std::endl;
    //     return availablePresentMode;
    //   }
    // }

    std::cout << "Present mode: V-Sync" << std::endl;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D MySwapChain::_chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) 
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else 
    {
        VkExtent2D actualExtent = m_vkWindowExtent;
        actualExtent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

VkFormat MySwapChain::findDepthFormat() 
{
    return m_myDevice.findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

//
// Picking
//
void MySwapChain::_createPickingResources()
{
    _createPickingImageBuffers();
    _createPickRenderPass();
    _createPickFramebuffers();
}

void MySwapChain::_createPickingImageBuffers()
{
    // Create color image
    VkImageCreateInfo imageInfoColor{};
    imageInfoColor.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfoColor.imageType = VK_IMAGE_TYPE_2D;
    imageInfoColor.extent.width = m_vkSwapChainExtent.width;
    imageInfoColor.extent.height = m_vkSwapChainExtent.height;
    imageInfoColor.extent.depth = 1;
    imageInfoColor.mipLevels = 1;
    imageInfoColor.arrayLayers = 1;
    imageInfoColor.format = VK_FORMAT_R32_SFLOAT;
    imageInfoColor.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfoColor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //imageInfoColor.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfoColor.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // note: VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT are used for picking debug purpose
    imageInfoColor.samples = VK_SAMPLE_COUNT_1_BIT; // Pick buffer doesn't need antialiasing
    imageInfoColor.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfoColor.flags = 0;

    m_myDevice.createImageWithInfo(
        imageInfoColor,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_vkPickColorImage,
        m_vkPickColorImageMemory);

    VkImageViewCreateInfo viewInfoColor{};
    viewInfoColor.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfoColor.image = m_vkPickColorImage;
    viewInfoColor.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfoColor.format = VK_FORMAT_R32_SFLOAT;
    viewInfoColor.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfoColor.subresourceRange.baseMipLevel = 0;
    viewInfoColor.subresourceRange.levelCount = 1;
    viewInfoColor.subresourceRange.baseArrayLayer = 0;
    viewInfoColor.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_myDevice.device(), &viewInfoColor, nullptr, &m_vkPickColorImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pick color image view!");
    }

    // Create depth image
    VkFormat depthFormat = findDepthFormat();

    //depthFormat = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo imageInfoDepth{};
    imageInfoDepth.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfoDepth.imageType = VK_IMAGE_TYPE_2D;
    imageInfoDepth.extent.width = m_vkSwapChainExtent.width;
    imageInfoDepth.extent.height = m_vkSwapChainExtent.height;
    imageInfoDepth.extent.depth = 1;
    imageInfoDepth.mipLevels = 1;
    imageInfoDepth.arrayLayers = 1;
    imageInfoDepth.format = depthFormat;
    imageInfoDepth.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfoDepth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfoDepth.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfoDepth.samples = VK_SAMPLE_COUNT_1_BIT;  // Pick buffer doesn't need antialiasing
    imageInfoDepth.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfoDepth.flags = 0;

    m_myDevice.createImageWithInfo(
        imageInfoDepth,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_vkPickDepthImage,
        m_vkPickDepthImageMemory);

    VkImageViewCreateInfo viewInfoDepth{};
    viewInfoDepth.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfoDepth.image = m_vkPickDepthImage;
    viewInfoDepth.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfoDepth.format = depthFormat;
    viewInfoDepth.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfoDepth.subresourceRange.baseMipLevel = 0;
    viewInfoDepth.subresourceRange.levelCount = 1;
    viewInfoDepth.subresourceRange.baseArrayLayer = 0;
    viewInfoDepth.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_myDevice.device(), &viewInfoDepth, nullptr, &m_vkPickDepthImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pick depth image view!");
    }
}

void MySwapChain::_createPickRenderPass()
{
    //color attachment with 1 msaa samples and a format of VK_FORMAT_R32_SFLOAT, we also clear both color and depth on load
    VkAttachmentDescription object_picking_colorAttachment{};
    object_picking_colorAttachment.format = VK_FORMAT_R32_SFLOAT;
    object_picking_colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    object_picking_colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    object_picking_colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    object_picking_colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    object_picking_colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    object_picking_colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    object_picking_colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //depth attachment with 1 msaa sample
    VkAttachmentDescription object_picking_depthAttachment{};
    object_picking_depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    object_picking_depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    object_picking_depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    object_picking_depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    object_picking_depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    object_picking_depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    object_picking_depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    object_picking_depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference object_picking_colorAttachmentRef{};
    object_picking_colorAttachmentRef.attachment = 0;
    object_picking_colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference object_picking_depthAttachmentRef{};
    object_picking_depthAttachmentRef.attachment = 1;
    object_picking_depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription object_picking_subpass{};
    object_picking_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    object_picking_subpass.colorAttachmentCount = 1;
    object_picking_subpass.pColorAttachments = &object_picking_colorAttachmentRef;
    object_picking_subpass.pDepthStencilAttachment = &object_picking_depthAttachmentRef;

    VkSubpassDependency object_picking_dependency{};
    object_picking_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    object_picking_dependency.dstSubpass = 0;
    object_picking_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    object_picking_dependency.srcAccessMask = 0;
    object_picking_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    object_picking_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2 > object_picking_attachments = { object_picking_colorAttachment,object_picking_depthAttachment };
    VkRenderPassCreateInfo object_picking_renderPassInfo{};
    object_picking_renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    object_picking_renderPassInfo.attachmentCount = static_cast<uint32_t>(object_picking_attachments.size());
    object_picking_renderPassInfo.pAttachments = object_picking_attachments.data();
    object_picking_renderPassInfo.subpassCount = 1;
    object_picking_renderPassInfo.pSubpasses = &object_picking_subpass;
    object_picking_renderPassInfo.dependencyCount = 1;
    object_picking_renderPassInfo.pDependencies = &object_picking_dependency;

    if (vkCreateRenderPass(m_myDevice.device(), &object_picking_renderPassInfo, nullptr, &m_vkPickRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pick render pass!");
    }
}

void MySwapChain::_createPickFramebuffers()
{
    std::array<VkImageView, 2> attachments = { m_vkPickColorImageView, m_vkPickDepthImageView };

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_vkPickRenderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = m_vkSwapChainExtent.width;
    framebufferInfo.height = m_vkSwapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(
        m_myDevice.device(),
        &framebufferInfo,
        nullptr,
        &m_vkPickFrameBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pick framebuffer!");
    }
}

