#include "my_offscreen_render_factory.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

MyOffScreenRenderFactory::MyOffScreenRenderFactory(MyDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout offscreenSetLayout)
    : m_myDevice{ device } 
{
    _createPipelineLayout(offscreenSetLayout);
    _createPipeline(renderPass);
}

MyOffScreenRenderFactory::~MyOffScreenRenderFactory()
{
    vkDestroyPipelineLayout(m_myDevice.device(), m_vkPipelineLayout, nullptr);
}

void MyOffScreenRenderFactory::_createPipelineLayout(VkDescriptorSetLayout offscreenSetLayout)
{
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(MySimplePushConstantData);

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ offscreenSetLayout };

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

void MyOffScreenRenderFactory::_createPipeline(VkRenderPass renderPass)
{
    assert(m_vkPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    MyPipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = m_vkPipelineLayout;
    pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
    pipelineConfig.rasterizationInfo.depthBiasEnable = VK_TRUE;
    pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    pipelineConfig.colorBlendInfo.attachmentCount = 0;
    pipelineConfig.colorBlendInfo.pAttachments = nullptr;

    pipelineConfig.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
    pipelineConfig.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineConfig.dynamicStateInfo.pDynamicStates = pipelineConfig.dynamicStateEnables.data();
    pipelineConfig.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(pipelineConfig.dynamicStateEnables.size());
    pipelineConfig.dynamicStateInfo.flags = 0;

    m_pMyPipeline = std::make_unique<MyPipeline>(
        m_myDevice,
        "shaders/offscreen_shader.vert.spv",
        pipelineConfig);
}

void MyOffScreenRenderFactory::renderGameObjects(MyFrameInfo& frameInfo)
{
    // Bind the descriptor set to the render pipeline
    m_pMyPipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(
        frameInfo.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_vkPipelineLayout,
        0, // only bind set 0 for now
        1, // count of 1
        &frameInfo.offscreenDescriptorSet,
        0,
        nullptr);

    // Now loop through all key-value pair of the game objects
    for (auto& kv : frameInfo.gameObjects) 
    {
	    auto& obj = kv.second;
        if (obj.type() == MyGameObject::DEBUG) continue;

        //if (obj.getID() == 2) continue;
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
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(MySimplePushConstantData),
            &push);

        obj.model->bind(frameInfo.commandBuffer);
        obj.model->draw(frameInfo.commandBuffer);
    }
}

void MyOffScreenRenderFactory::recratePipeline(VkRenderPass renderPass)
{
    // delete the original one if exists

    std::shared_ptr<MyPipeline> oldPipeline = std::move(m_pMyPipeline);

    _createPipeline(renderPass);

    oldPipeline = nullptr;
}
