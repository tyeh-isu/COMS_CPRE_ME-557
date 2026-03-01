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

	enum MyAppKeyMap
	{
		KEY_NONE = 0,
		KEY_LEFT,
		KEY_RIGHT,
		KEY_UP,
		KEY_DOWN,
		KEY_FORWARD,
		KEY_BACKWARD
	};

	MyApplication();

	void run();
	void switchProjectionMatrix();
	void handleMovement(MyAppKeyMap key);

private:
	void _loadGameObjects();

	MyWindow                  m_myWindow{ WIDTH, HEIGHT, "Hello Vulkan!" };
	MyDevice                  m_myDevice{ m_myWindow };
	MyRenderer                m_myRenderer{ m_myWindow, m_myDevice };

	std::vector<MyGameObject> m_vMyGameObjects;
	bool                      m_bPerspectiveProjection;
	float                     m_cameraviewXYZ[3];
};

#endif

