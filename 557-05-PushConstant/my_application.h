#ifndef __MY_APPLICATION_H__
#define __MY_APPLICATION_H__

#include "my_window.h"
#include "my_pipeline.h"
#include "my_device.h"
#include "my_swap_chain.h"

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
	void _createPipelineLayout();
    void _createPipeline();
	void _createCommandBuffers();
	void _drawFrame();
    void _recordCommandBuffer(int imageIndex);

	MyWindow                     m_myWindow{ WIDTH, HEIGHT, "Hello Vulkan!" };
	MyDevice                     m_myDevice{ m_myWindow };
	MySwapChain                  m_mySwapChain{ m_myDevice, m_myWindow.extent() };

	std::unique_ptr<MyPipeline>  m_pMyPipeline;
	
	VkPipelineLayout             m_vkPipelineLayout;
	std::vector<VkCommandBuffer> m_vVkCommandBuffers;
};

#endif

