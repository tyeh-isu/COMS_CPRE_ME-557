#include "my_renderer.h"

// std
#include <array>
#include <cassert>
#include <stdexcept>

MyRenderer::MyRenderer(MyWindow& window, MyDevice& device)
    : m_myWindow{ window },
	  m_myDevice{ device }
{
    m_iCurrentImageIndex = 0;
    m_iCurrentFrameIndex = 0;
    m_bIsFrameStarted = false;

    _recreateSwapChain();
    _createCommandBuffers();
}

MyRenderer::~MyRenderer() 
{ 
    _freeCommandBuffers();
}

void MyRenderer::_recreateSwapChain()
{
    // make sure the window's size is not 0
    auto extent = m_myWindow.extent();
    while (extent.width == 0 || extent.height == 0)
    {
        extent = m_myWindow.extent();
        m_myWindow.waitEvents();
    }

    vkDeviceWaitIdle(m_myDevice.device());

    if (m_mySwapChain == nullptr)
    {
        m_mySwapChain = std::make_unique<MySwapChain>(m_myDevice, extent);
    }
    else
    {
        std::shared_ptr<MySwapChain> oldSwapChain = std::move(m_mySwapChain);
        m_mySwapChain = std::make_unique<MySwapChain>(m_myDevice, extent, oldSwapChain);

        if (!oldSwapChain->compareSwapFormats(*m_mySwapChain.get()))
        {
            throw std::runtime_error("Swap chain image(or depth) format has changed!");
        }
    }
}

void MyRenderer::_createCommandBuffers()
{
    m_vVkCommandBuffers.resize(MySwapChain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_myDevice.commandPool();
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_vVkCommandBuffers.size());

    if (vkAllocateCommandBuffers(m_myDevice.device(), &allocInfo, m_vVkCommandBuffers.data()) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void MyRenderer::_freeCommandBuffers()
{
    vkFreeCommandBuffers(
        m_myDevice.device(),
        m_myDevice.commandPool(),
        static_cast<uint32_t>(m_vVkCommandBuffers.size()),
        m_vVkCommandBuffers.data());

    m_vVkCommandBuffers.clear();
}

void MyRenderer::recreateSwapChain()
{
    _recreateSwapChain();
}

VkCommandBuffer MyRenderer::beginFrame()
{
    assert(!m_bIsFrameStarted && "Can't call beginFrame while already in progress");

    auto result = m_mySwapChain->acquireNextImage(&m_iCurrentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        //m_myWindow.resetWindowResizedFlag();
        _recreateSwapChain();

        // Wait until all operations on the device have been finished before destroying resources
        /*vkDeviceWaitIdle(m_myDevice.device());

        _recreateSwapChain();
        _freeCommandBuffers();
        _createCommandBuffers();

        vkDeviceWaitIdle(m_myDevice.device());*/

        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    m_bIsFrameStarted = true;

    auto commandBuffer = _currentCommandBuffer();
    //vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT); // optional

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    return commandBuffer;
}

int MyRenderer::endFrame()
{
    int retVal = 0;

    assert(m_bIsFrameStarted && "Can't call endFrame while frame is not in progress");
    auto commandBuffer = _currentCommandBuffer();
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }

    auto result = m_mySwapChain->submitCommandBuffers(&commandBuffer, &m_iCurrentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_myWindow.wasWindowResized())
    {
        vkDeviceWaitIdle(m_myDevice.device());
        m_myWindow.resetWindowResizedFlag();
		_recreateSwapChain();
        //_freeCommandBuffers();
        //m_myDevice.resetCommandPool();
        //_createCommandBuffers();
        
		vkDeviceWaitIdle(m_myDevice.device());

        retVal = 1;
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_bIsFrameStarted = false;
    m_iCurrentFrameIndex = (m_iCurrentFrameIndex + 1) % MySwapChain::MAX_FRAMES_IN_FLIGHT;

    return retVal;
}

void MyRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
    assert(m_bIsFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
    assert(
        commandBuffer == _currentCommandBuffer() &&
        "Can't begin render pass on command buffer from a different frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_mySwapChain->renderPass();
    renderPassInfo.framebuffer = m_mySwapChain->frameBuffer(m_iCurrentImageIndex);

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_mySwapChain->swapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_mySwapChain->swapChainExtent().width);
    viewport.height = static_cast<float>(m_mySwapChain->swapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{ {0, 0}, m_mySwapChain->swapChainExtent() };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void MyRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) 
{
    assert(m_bIsFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
    assert(
        commandBuffer == _currentCommandBuffer() &&
        "Can't end render pass on command buffer from a different frame");
    vkCmdEndRenderPass(commandBuffer);
}

void MyRenderer::beginOffscreenRenderPass(VkCommandBuffer commandBuffer)
{
    assert(m_bIsFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
    assert(
        commandBuffer == _currentCommandBuffer() &&
        "Can't begin render pass on command buffer from a different frame");

    VkClearValue clearValues[2];
    clearValues[0].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = m_mySwapChain->shadowMapRenderPass();
    renderPassBeginInfo.framebuffer = m_mySwapChain->shadowMapFrameBuffer();
    renderPassBeginInfo.renderArea.extent.width = m_mySwapChain->m_vShadowMapSize[0];
    renderPassBeginInfo.renderArea.extent.height = m_mySwapChain->m_vShadowMapSize[1];
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.width = (float)m_mySwapChain->m_vShadowMapSize[0];
    viewport.height = (float)m_mySwapChain->m_vShadowMapSize[1];
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent.width = m_mySwapChain->m_vShadowMapSize[0];;
    scissor.extent.height = m_mySwapChain->m_vShadowMapSize[1];;
    scissor.offset.x = 0;
    scissor.offset.y = 0;

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Set depth bias (aka "Polygon offset")
    // Required to avoid shadow mapping artifacts
    // Depth bias (and slope) are used to avoid shadowing artifacts
    // Constant depth bias factor (always applied)
    float depthBiasConstant = 1.25f;
    // Slope depth bias factor, applied depending on polygon's slope
    float depthBiasSlope = 1.75f;

    vkCmdSetDepthBias(
        commandBuffer,
        depthBiasConstant,
        0.0f,
        depthBiasSlope);
}

void MyRenderer::endOffscreenRenderPass(VkCommandBuffer commandBuffer)
{
    vkCmdEndRenderPass(commandBuffer);
}

VkExtent2D MyRenderer::swapChainExtent()
{
    return m_mySwapChain->swapChainExtent();
}

// Pick
void MyRenderer::beginPickRenderPass(VkCommandBuffer commandBuffer, int x, int y, bool bDebug)
{
    assert(m_bIsFrameStarted && "Can't call beginPickRenderPass if frame is not in progress");
    assert(
        commandBuffer == _currentCommandBuffer() &&
        "Can't begin render pass on command buffer from a different frame");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_mySwapChain->pickRenderPass();
    renderPassInfo.framebuffer = m_mySwapChain->pickFrameBuffer();

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_mySwapChain->swapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.00f, 0.00f, 0.00f, 1.0f }; // Clear background as black
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkRect2D rect{};
 
    if (bDebug)
    {
        rect.offset.x = 0;
        rect.offset.y = 0;
        rect.extent = m_mySwapChain->swapChainExtent();
    }
    else
    {
        rect.offset.x = x;
        rect.offset.y = y;
        rect.extent = { 1, 1 };
    }

    vkCmdSetScissor(commandBuffer, 0, 1, &rect);
}

void MyRenderer::endPickRenderPass(VkCommandBuffer commandBuffer)
{
    assert(m_bIsFrameStarted && "Can't call endPickRenderPass if frame is not in progress");
    assert(
        commandBuffer == _currentCommandBuffer() &&
        "Can't end render pass on command buffer from a different frame");
    vkCmdEndRenderPass(commandBuffer);
}

