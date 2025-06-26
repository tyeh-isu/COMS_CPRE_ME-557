#ifndef __MY_DEBUG_RENDER_FACTORY_H__
#define __MY_DEBUG_RENDER_FACTORY_H__

#include "my_device.h"
#include "my_game_object.h"
#include "my_pipeline.h"
#include "my_camera.h"
#include "my_frame_info.h"

// std
#include <memory>
#include <vector>

class MyDebugRenderFactory
{
public:
	MyDebugRenderFactory(MyDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~MyDebugRenderFactory();

	MyDebugRenderFactory(const MyDebugRenderFactory&) = delete;
	MyDebugRenderFactory& operator=(const MyDebugRenderFactory&) = delete;
	MyDebugRenderFactory(MyDebugRenderFactory&&) = delete;
	MyDebugRenderFactory& operator=(const MyDebugRenderFactory&&) = delete;

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

