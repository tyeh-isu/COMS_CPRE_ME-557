#ifndef __MY_APPLICATION_H__
#define __MY_APPLICATION_H__

#include "my_window.h"

class MyApplication 
{
public:
	static constexpr int WIDTH = 800;
	static constexpr int HEIGHT = 600;

	void run();

private:
	MyWindow   m_myWindow{ WIDTH, HEIGHT, "Hello Vulkan!" };
};

#endif

