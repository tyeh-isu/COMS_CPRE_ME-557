#ifndef __MY_APPLICATION_H__
#define __MY_APPLICATION_H__

#include "my_window.h"
#include "my_pipeline.h"
#include "my_device.h"
#include <memory>

class MyApplication 
{
public:
	static constexpr int WIDTH = 800;
	static constexpr int HEIGHT = 600;

	// Rule of zero - no need to define the destructor in this case
	MyApplication();

	void run();

private:
    void _createPipeline();

	MyWindow                     m_myWindow{ WIDTH, HEIGHT, "Hello Vulkan!" };
	MyDevice                     m_myDevice{ m_myWindow };
	std::unique_ptr<MyPipeline>  m_pMyPipeline;
};

#endif

