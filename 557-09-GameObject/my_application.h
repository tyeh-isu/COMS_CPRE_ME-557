#ifndef __MY_APPLICATION_H__
#define __MY_APPLICATION_H__

#include "my_window.h"
#include "my_pipeline.h"
#include "my_device.h"
#include "my_swap_chain.h"
#include "my_game_object.h"

#include <string>
#include <memory>
#include <vector>

class MyApplication 
{
public:
	static constexpr int WIDTH = 800;
	static constexpr int HEIGHT = 600;

	MyApplication();
	~MyApplication();

	// C++ rule of five
	MyApplication(const MyApplication&) = delete;
	MyApplication& operator=(const MyApplication&) = delete;
	MyApplication(MyApplication &&) = delete;
	MyApplication& operator=(const MyApplication&&) = delete;

	void run();

private:
	void _loadGameObjects();
	void _createPipelineLayout();
    void _createPipeline();
	void _createCommandBuffers();
	void _freeCommandBuffers();
	void _drawFrame();
	void _recreateSwpChain();
	void _recordCommandBuffer(int imageIndex);
	void _renderGameObjects(VkCommandBuffer commandBuffer);

	MyWindow                     m_myWindow{ WIDTH, HEIGHT, "Hello Vulkan!" };
	MyDevice                     m_myDevice{ m_myWindow };

	std::unique_ptr<MySwapChain> m_pMySwapChain; // change to unique pointer so we can recreate one after window resize
	std::unique_ptr<MyPipeline>  m_pMyPipeline;
	
	VkPipelineLayout             m_vkPipelineLayout;
	std::vector<VkCommandBuffer> m_vVkCommandBuffers;
	std::vector<MyGameObject>    m_vMyGameObjects;
};

#endif

