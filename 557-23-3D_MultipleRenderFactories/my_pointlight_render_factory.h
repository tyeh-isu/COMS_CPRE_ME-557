#ifndef __MY_POINTLIGHT_RENDER_FACTORY_H__
#define __MY_POINTLIGHT_RENDER_FACTORY_H__

#include "my_camera.h"
#include "my_device.h"
#include "my_frame_info.h"
#include "my_game_object.h"
#include "my_pipeline.h"

// std
#include <memory>
#include <vector>

class MyPointLightRenderFactory
{
public:
    MyPointLightRenderFactory(
        MyDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~MyPointLightRenderFactory();

	MyPointLightRenderFactory(const MyPointLightRenderFactory&) = delete;
	MyPointLightRenderFactory& operator=(const MyPointLightRenderFactory&) = delete;
	MyPointLightRenderFactory(MyPointLightRenderFactory&&) = delete;
    MyPointLightRenderFactory& operator=(const MyPointLightRenderFactory&&) = delete;
	
	void render(MyFrameInfo& frameInfo);

private:
	void _createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void _createPipeline(VkRenderPass renderPass);

	MyDevice&                   m_myDevice;

	std::unique_ptr<MyPipeline> m_pMyPipeline;
	VkPipelineLayout            m_vkPipelineLayout;
};

#endif

