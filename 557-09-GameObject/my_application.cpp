#include "my_application.h"

// use radian rather degree for angle
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// Std
#include <stdexcept>
#include <array>

struct SimplePushConstantData
{
    glm::mat2 transform{ 1.0f };
    glm::vec2 offset;
    alignas(16) glm::vec3 push_color;
};

MyApplication::MyApplication()
{
    _loadGameObjects();
    _createPipelineLayout();
    //_createPipeline();
    _recreateSwpChain(); // this will call _createPipeline
    _createCommandBuffers();
}

MyApplication::~MyApplication()
{
    vkDestroyPipelineLayout(m_myDevice.device(), m_vkPipelineLayout, nullptr);
}

void MyApplication::run() 
{
    while (!m_myWindow.shouldClose()) 
    {
        // Note: depending on the platform (Windows, Linux or Mac), this function
        // will cause the event proecssing to block during a Window move, resize or
        // menu operation. Users can use the "window refresh callback" to redraw the
        // contents of the window when necessary during such operation.
        m_myWindow.pollEvents();
        _drawFrame();
    }

    // GPU will block until all CPU is complete
    vkDeviceWaitIdle(m_myDevice.device());
}

void MyApplication::_loadGameObjects()
{
    // add postition and color interleaved vertex buffer
    std::vector<MyModel::Vertex> vertices
	{
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

    auto model = std::make_shared<MyModel>(m_myDevice, vertices);

    auto triangle = MyGameObject::createGameObject();
    triangle.model = model;
    triangle.color = { 0.1f, 0.8f, 0.1f };
    triangle.transform2d.translation.x = 0.2f;
    triangle.transform2d.scale = {2.0f, 0.5f};
    triangle.transform2d.rotation = 0.25f * glm::two_pi<float>(); // 90 degree rotation

    m_vMyGameObjects.push_back(std::move(triangle));
}

void MyApplication::_createPipelineLayout()
{
    VkPushConstantRange pushContantRange{};

    // We would like to push the constant data to both vertex shader and fragment shader
    pushContantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushContantRange.offset = 0; // used if you are using separated ranges for the vertex and fragment shaders
    pushContantRange.size = sizeof(SimplePushConstantData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushContantRange;

    if (vkCreatePipelineLayout(m_myDevice.device(), &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline layout!");
    }
}

void MyApplication::_createPipeline()
{
    // Note: the swap chain buffer size may not be the same as the Window size
    // Pipeline config can be considered as a blue print to create graphics pipeline
    PipelineConfigInfo pipelineConfig{};
        
    // Please note that the pipeline depends on the size of the swap chain
    // the swap chain depends on the window size
    // so when window resize, we need to create a new swap chain
    // and then create a new pipeline
    MyPipeline::defaultPipelineConfigInfo(pipelineConfig);

    // For now render pass contains the structure and the format
    // of the frame buffer object and its attachments
    // for example in the defect FBO, attachment 0 is the color buffer
    // and attachment 1 is the depth buffer
    pipelineConfig.renderPass = m_pMySwapChain->renderPass();
    pipelineConfig.pipelineLayout = m_vkPipelineLayout;

    m_pMyPipeline = std::make_unique< MyPipeline >(
        m_myDevice,
        "shaders/simple_shader.vert.spv",
        "shaders/simple_shader.frag.spv",
        pipelineConfig
        );
}

void MyApplication::_createCommandBuffers()
{
    // Command buffers contain one or multiple commends that are to be executed on the GPU
    // 
    // 1:1 mapping between the command buffer and the frame buffer for now
    // Could be either 2 or 3 depending on the graphics hardware
    m_vVkCommandBuffers.resize(m_pMySwapChain->imageCount());

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

void MyApplication::_freeCommandBuffers()
{
    vkFreeCommandBuffers(m_myDevice.device(), m_myDevice.commandPool(), static_cast<uint32_t>(m_vVkCommandBuffers.size()), m_vVkCommandBuffers.data());
    m_vVkCommandBuffers.clear();
}

void MyApplication::_recreateSwpChain()
{
    auto extent = m_myWindow.extent();

    // Wait until we have a valid window
    // or the program just wait when mininizing the window
    while (extent.width == 0 || extent.height == 0)
    {
        extent = m_myWindow.extent();
        glfwWaitEvents(); // Window will pause and wait for the next event
    }

    // Wait until the current swap chain is not used anymore so
    // we can create another swap chain
    vkDeviceWaitIdle(m_myDevice.device());

    // Release the old one
    if (m_pMySwapChain == nullptr)
    {
        // Create a new swap chain based on the new window width and height
        m_pMySwapChain = std::make_unique<MySwapChain>(m_myDevice, extent);
    }
    else
    {
        m_pMySwapChain = std::make_unique<MySwapChain>(m_myDevice, extent, std::move(m_pMySwapChain));
        
        if (m_pMySwapChain->imageCount() != m_vVkCommandBuffers.size())
        {
            _freeCommandBuffers();
            _createCommandBuffers();
        }
    }

    // Since the pipeline depends on swap chain and its render pass, 
    // we need to re-create another pipeline again if render pass changes
    // However, if render pass is compatible, we don't need to recreate a new pipeline
    // So some optimization can be done later here.
    // Meaning if another render pass is compatible, do nothing here
    _createPipeline();
}

void MyApplication::_recordCommandBuffer(int imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(m_vVkCommandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffers!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_pMySwapChain->renderPass();
    renderPassInfo.framebuffer = m_pMySwapChain->frameBuffer(imageIndex);
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_pMySwapChain->swapChainExtent();

    // becuse index 0 is color buffer and 1 is depth buffer
    // so we only need to clear the proper index only
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.1f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // VK_SUBPASS_CONTENTS_INLINE means only use Primary command buffer. No secondary command buffer
    vkCmdBeginRenderPass(m_vVkCommandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_pMySwapChain->swapChainExtent().width);
    viewport.height = static_cast<float>(m_pMySwapChain->swapChainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{ {0, 0}, m_pMySwapChain->swapChainExtent() };
    vkCmdSetViewport(m_vVkCommandBuffers[imageIndex], 0, 1, &viewport);
    vkCmdSetScissor(m_vVkCommandBuffers[imageIndex], 0, 1, &scissor);

    _renderGameObjects(m_vVkCommandBuffers[imageIndex]);

    vkCmdEndRenderPass(m_vVkCommandBuffers[imageIndex]);
    if (vkEndCommandBuffer(m_vVkCommandBuffers[imageIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to recording command buffers!");
    }
}

void MyApplication::_drawFrame()
{
    uint32_t imageIndex = 0;
    auto result = m_pMySwapChain->acquireNextImage(&imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) // this error could occur after the window is resized
    {
        _recreateSwpChain();
        return;
    }

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    _recordCommandBuffer(imageIndex);

    // Submit the command buffer to the graphics queue
    // the GPU and CPU synchronization will be handled properly
    result = m_pMySwapChain->submitCommandBuffers(&m_vVkCommandBuffers[imageIndex], &imageIndex);

    // If window resize
    // SUBOPTIMAL_KHR means that a swapchain no longer matches the surface properties exactly, but can
    // be used to present to teh surface successfully
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_myWindow.wasWindowResized())
    {
        m_myWindow.resetWindowResizedFlag();
        _recreateSwpChain();
        return;
    }

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit comamnd buffers!");
    }
}

void MyApplication::_renderGameObjects(VkCommandBuffer commandBuffer)
{
    m_pMyPipeline->bind(commandBuffer);

    for (auto& obj : m_vMyGameObjects)
    {
        obj.transform2d.rotation = glm::mod(obj.transform2d.rotation + 0.0001f, glm::two_pi<float>());

        SimplePushConstantData pushdata{};
        pushdata.offset = obj.transform2d.translation;
        pushdata.push_color = obj.color;
        pushdata.transform = obj.transform2d.mat2();

        vkCmdPushConstants(commandBuffer,
            m_vkPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(SimplePushConstantData), &pushdata);

        obj.model->bind(commandBuffer);
        obj.model->draw(commandBuffer);

    }
}

