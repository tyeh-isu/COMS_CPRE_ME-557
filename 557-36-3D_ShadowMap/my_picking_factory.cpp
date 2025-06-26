#include "my_picking_factory.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

MyPickingFactory::MyPickingFactory(MyDevice& device, MyRenderer &renderer, VkDescriptorSetLayout globalSetLayout)
    : m_myDevice{ device } 
{
    _createPipelineLayout(globalSetLayout);
    _createPipeline(renderer);
}

MyPickingFactory::~MyPickingFactory()
{
    vkDestroyPipelineLayout(m_myDevice.device(), m_vkPipelineLayout, nullptr);
}

void MyPickingFactory::_createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(MySimplePushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(m_myDevice.device(), &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

void MyPickingFactory::_createPipeline(MyRenderer& renderer)
{
    assert(m_vkPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    MyPipeline::defaultPipelineConfigInfo(pipelineConfig);

    // Viewport
    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)renderer.swapChainExtent().width;
    viewport.height = (float)renderer.swapChainExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0,0 };
    scissor.extent = renderer.swapChainExtent();

    pipelineConfig.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineConfig.viewportInfo.viewportCount = 1;
    pipelineConfig.viewportInfo.pViewports = &viewport;
    pipelineConfig.viewportInfo.scissorCount = 1;
    pipelineConfig.viewportInfo.pScissors = &scissor;

    pipelineConfig.renderPass = renderer.pickRenderPass();
    pipelineConfig.pipelineLayout = m_vkPipelineLayout;

    // Rasterizer // should be the same as the regular rendering pipeline
    /*pipelineConfig.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineConfig.rasterizationInfo.depthClampEnable = VK_FALSE;
    pipelineConfig.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
    pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineConfig.rasterizationInfo.lineWidth = 1.0f;
    pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineConfig.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; //VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineConfig.rasterizationInfo.depthBiasEnable = VK_FALSE;
    pipelineConfig.rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
    pipelineConfig.rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
    pipelineConfig.rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional*/

    // Dynamic state
    pipelineConfig.dynamicStateEnables.clear();
    pipelineConfig.dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);

    pipelineConfig.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineConfig.dynamicStateInfo.dynamicStateCount = (uint32_t)pipelineConfig.dynamicStateEnables.size();
    pipelineConfig.dynamicStateInfo.pDynamicStates = pipelineConfig.dynamicStateEnables.data();

    m_pMyPipeline = std::make_unique<MyPipeline>(
        m_myDevice,
        "shaders/pick_shader.vert.spv",
        "shaders/pick_shader.frag.spv",
        pipelineConfig);
}

//
// should be the same as the simple rendering
//
void MyPickingFactory::renderPickScene(MyFrameInfo& frameInfo)
{
    // Bind the descriptor set to the render pipeline
    m_pMyPipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(
        frameInfo.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_vkPipelineLayout,
        0, // only bind set 0 for now
        1, // count of 1
        &frameInfo.globalDescriptorSet,
        0,
        nullptr);

    // Now loop through all key-value pair of the game objects
    for (auto& kv : frameInfo.gameObjects) 
    {
	    auto& obj = kv.second;
        if (obj.type() == MyGameObject::DEBUG) continue;

        // Note: X to the right, Y up and Z out of the screen
        // obj.transform.rotation.y = glm::mod(obj.transform.rotation.y + 0.0005f, glm::two_pi<float>());  // Y up
        // obj.transform.rotation.x = glm::mod(obj.transform.rotation.x + 0.0005f, glm::two_pi<float>());  // X to the right
        // obj.transform.rotation.z = glm::mod(obj.transform.rotation.z + 0.0005f, glm::two_pi<float>());  // Z out of the screen

        MySimplePushConstantData push{};

        // Note: do this for now to perform on CPU
        // We will do it later to perform it on GPU
        push.modelMatrix = obj.transform.mat4();
        push.normalMatrix = obj.transform.normalMatrix();

        vkCmdPushConstants(
            frameInfo.commandBuffer,
            m_vkPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(MySimplePushConstantData),
            &push);

        obj.model->bind(frameInfo.commandBuffer);
        obj.model->draw(frameInfo.commandBuffer);
    }
}

void MyPickingFactory::recratePipeline(MyRenderer &render)
{
    // delete the original one if exists
    m_pMyPipeline = nullptr;

    _createPipeline(render);
}

