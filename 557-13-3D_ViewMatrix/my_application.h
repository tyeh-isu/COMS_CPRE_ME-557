#ifndef __MY_APPLICATION_H__
#define __MY_APPLICATION_H__

#include "my_window.h"
#include "my_device.h"
#include "my_renderer.h"
#include "my_game_object.h"

#include <memory>
#include <vector>

class MyApplication 
{
public:
	static constexpr int WIDTH = 800;
	static constexpr int HEIGHT = 600;

	MyApplication();

	void run();
	void switchProjectionMatrix();

private:
	void _loadGameObjects();

	MyWindow                  m_myWindow{ WIDTH, HEIGHT, "Hello Vulkan!" };
	MyDevice                  m_myDevice{ m_myWindow };
	MyRenderer                m_myRenderer{ m_myWindow, m_myDevice };

	std::vector<MyGameObject> m_vMyGameObjects;
	bool                      m_bPerspectiveProjection;
};

#endif

