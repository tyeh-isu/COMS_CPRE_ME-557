#include "my_application.h"
#include <stdexcept>
#include <array>

struct SimplePushConstantData
{
    float time;
};

MyApplication::MyApplication()
{
    _loadModel();
    _createPipelineLayout();
    _createPipeline();
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
        glfwPollEvents();
        _drawFrame();
    }

    // GPU will block until all CPU is complete
    vkDeviceWaitIdle(m_myDevice.device());
}

void MyApplication::_loadModel()
{
    std::vector<MyModel::Vertex> vertices {
        {{0.0f, -0.5f}},
        {{0.5f, 0.5f}},
        {{-0.5f, 0.5f}}
    };

    m_pMyModel = std::make_unique<MyModel>(m_myDevice, vertices);
}

void MyApplication::_createPipelineLayout()
{
    VkPushConstantRange pushContantRange{};

    // We would like to push the constant data to vertex shader only
    pushContantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //  | VK_SHADER_STAGE_FRAGMENT_BIT;
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
        
    MyPipeline::defaultPipelineConfigInfo(pipelineConfig, m_mySwapChain.width(), m_mySwapChain.height());

    // For now render pass contains the structure and the format
    // of the frame buffer object and its attachments
    // for example in the defect FBO, attachment 0 is the color buffer
    // and attachment 1 is the depth buffer
    pipelineConfig.renderPass = m_mySwapChain.renderPass();
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
    m_vVkCommandBuffers.resize(m_mySwapChain.imageCount());

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
    renderPassInfo.renderPass = m_mySwapChain.renderPass();
    renderPassInfo.framebuffer = m_mySwapChain.frameBuffer(imageIndex);
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_mySwapChain.swapChainExtent();

    // becuse index 0 is color buffer and 1 is depth buffer
    // so we only need to clear the proper index only
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.1f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // VK_SUBPASS_CONTENTS_INLINE means only use Primary command buffer. No secondary command buffer
    vkCmdBeginRenderPass(m_vVkCommandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    m_pMyPipeline->bind(m_vVkCommandBuffers[imageIndex]);

    static float step = 0.0f;
    static float sign = 1.0f;

    SimplePushConstantData pushdata{};
    
    // Step value goes between -1.0f and 1.0f
    if (step >= 1.0f)
      { sign = -1.0f; }
    else if (step <= -1.0f)
      { sign =  1.0f; }

    // 0.0001f can be considered as the moving speeed
    step += 0.0001f * sign;

    pushdata.time = step;

    vkCmdPushConstants(m_vVkCommandBuffers[imageIndex],
                       m_vkPipelineLayout, 
					   VK_SHADER_STAGE_VERTEX_BIT,  // push to vertex shader only
                       //VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(SimplePushConstantData), &pushdata);

    m_pMyModel->bind(m_vVkCommandBuffers[imageIndex]);
    m_pMyModel->draw(m_vVkCommandBuffers[imageIndex]);

    vkCmdEndRenderPass(m_vVkCommandBuffers[imageIndex]);
    if (vkEndCommandBuffer(m_vVkCommandBuffers[imageIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to recording command buffers!");
    }
}

void MyApplication::_drawFrame()
{
    uint32_t imageIndex = 0;
    auto result = m_mySwapChain.acquireNextImage(&imageIndex);

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    _recordCommandBuffer(imageIndex);

    // Submit the command buffer to the graphics queue
    // the GPU and CPU synchronization will be handled properly
    result = m_mySwapChain.submitCommandBuffers(&m_vVkCommandBuffers[imageIndex], &imageIndex);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit comamnd buffers!");
    }
}

