#ifndef __MY_SIMPLE_RENDER_SYSTEM_H__
#define __MY_SIMPLE_RENDER_SYSTEM_H__

#include "my_device.h"
#include "my_game_object.h"
#include "my_pipeline.h"
#include "my_camera.h"
#include "my_frame_info.h"

// std
#include <memory>
#include <vector>


class MySimpleRenderSystem
{
public:
	MySimpleRenderSystem(MyDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~MySimpleRenderSystem();

	MySimpleRenderSystem(const MySimpleRenderSystem&) = delete;
	MySimpleRenderSystem& operator=(const MySimpleRenderSystem&) = delete;

	void renderGameObjects(MyFrameInfo &frameInfo);

private:
	void _createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void _createPipeline(VkRenderPass renderPass);

	MyDevice&                   m_myDevice;

	std::unique_ptr<MyPipeline> m_pMyPipeline;
	VkPipelineLayout            m_vkPipelineLayout;
};

#endif

