#ifndef __MY_TEXTURE_RENDER_FACTORY_H__
#define __MY_TEXTURE_RENDER_FACTORY_H__

#include "my_camera.h"
#include "my_device.h"
#include "my_frame_info.h"
#include "my_game_object.h"
#include "my_pipeline.h"

// std
#include <memory>
#include <vector>

class MyTextureRenderFactory
{
public:
	MyTextureRenderFactory(MyDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~MyTextureRenderFactory();

	MyTextureRenderFactory(const MyTextureRenderFactory&) = delete;
	MyTextureRenderFactory& operator=(const MyTextureRenderFactory&) = delete;
	MyTextureRenderFactory(MyTextureRenderFactory&&) = delete;
	MyTextureRenderFactory& operator=(const MyTextureRenderFactory&&) = delete;

	void render(MyFrameInfo& frameInfo);
	void recratePipeline(VkRenderPass renderPass);

private:
	void _createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void _createPipeline(VkRenderPass renderPass);

	MyDevice& m_myDevice;

	std::unique_ptr<MyPipeline> m_pMyPipeline;
	VkPipelineLayout            m_vkPipelineLayout;
};

#endif

