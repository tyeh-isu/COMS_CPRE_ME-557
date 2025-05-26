#ifndef __MY_APPLICATION_H__
#define __MY_APPLICATION_H__

#include "my_window.h"
#include "my_pipeline.h"
#include <string>

class MyApplication 
{
public:
	static constexpr int WIDTH = 800;
	static constexpr int HEIGHT = 600;

	void run();

private:
	MyWindow   m_myWindow{ WIDTH, HEIGHT, "Hello Vulkan!" };
	MyPipeline m_myPipeline{ "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv" };
};

#endif

