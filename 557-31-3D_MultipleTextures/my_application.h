#ifndef __MY_APPLICATION_H__
#define __MY_APPLICATION_H__

#include "my_descriptors.h"
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
	void applyNextTexture();

private:
	void _loadGameObjects();

	MyWindow                  m_myWindow{ WIDTH, HEIGHT, "Hello Vulkan!" };
	MyDevice                  m_myDevice{ m_myWindow };
	MyRenderer                m_myRenderer{ m_myWindow, m_myDevice };

	// Note: the order matters, because the destructor is called in the reversed order
	// globalPool needs to delete before m_myDevice
	std::unique_ptr<MyDescriptorPool> m_pMyGlobalPool{};
	MyGameObject::Map                 m_mapGameObjects;
	bool                              m_bPerspectiveProjection;
	unsigned int                      m_iTextureID;
};

#endif

