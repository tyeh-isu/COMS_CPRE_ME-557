#ifndef __MY_PICKING_FACTORY_H__
#define __MY_PICKING_FACTORY_H__

#include "my_device.h"
#include "my_game_object.h"
#include "my_pipeline.h"
#include "my_camera.h"
#include "my_renderer.h"
#include "my_frame_info.h"

// std
#include <memory>
#include <vector>

class MyPickingFactory
{
public:
	MyPickingFactory(MyDevice& device, MyRenderer &renderer, VkDescriptorSetLayout globalSetLayout);
	~MyPickingFactory();

	MyPickingFactory(const MyPickingFactory&) = delete;
	MyPickingFactory& operator=(const MyPickingFactory&) = delete;
	MyPickingFactory(MyPickingFactory&&) = delete;
	MyPickingFactory& operator=(const MyPickingFactory&&) = delete;

	void renderPickScene(MyFrameInfo& frameInfo);

private:
	void _createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void _createPipeline(MyRenderer &render);

	MyDevice&                   m_myDevice;

	std::unique_ptr<MyPipeline> m_pMyPipeline;
	VkPipelineLayout            m_vkPipelineLayout;
};

#endif

