#ifndef __MY_OFFSCREEN_RENDER_FACTORY_H__
#define __MY_OFFSCREEN_RENDER_FACTORY_H__

#include "my_device.h"
#include "my_game_object.h"
#include "my_pipeline.h"
#include "my_camera.h"
#include "my_frame_info.h"

// std
#include <memory>
#include <vector>

class MyOffScreenRenderFactory
{
public:
	MyOffScreenRenderFactory(MyDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~MyOffScreenRenderFactory();

	MyOffScreenRenderFactory(const MyOffScreenRenderFactory&) = delete;
	MyOffScreenRenderFactory& operator=(const MyOffScreenRenderFactory&) = delete;
	MyOffScreenRenderFactory(MyOffScreenRenderFactory&&) = delete;
	MyOffScreenRenderFactory& operator=(const MyOffScreenRenderFactory&&) = delete;

	void renderGameObjects(MyFrameInfo &frameInfo);
	void recratePipeline(VkRenderPass renderPass);

private:
	void _createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void _createPipeline(VkRenderPass renderPass);

	MyDevice&                   m_myDevice;

	std::unique_ptr<MyPipeline> m_pMyPipeline;
	VkPipelineLayout            m_vkPipelineLayout;
};

#endif

