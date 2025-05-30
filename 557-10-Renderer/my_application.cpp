#include "my_application.h"

// Render system
#include "my_simple_render_factory.h"

// use radian rather degree for angle
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// Std
#include <stdexcept>
#include <array>

MyApplication::MyApplication()
{
    _loadGameObjects();
}

void MyApplication::run() 
{
    MySimpleRenderFactory simpleRenderFactotry{ m_myDevice, m_myRenderer.swapChainRenderPass() };

    while (!m_myWindow.shouldClose()) 
    {
        // Note: depending on the platform (Windows, Linux or Mac), this function
        // will cause the event proecssing to block during a Window move, resize or
        // menu operation. Users can use the "window refresh callback" to redraw the
        // contents of the window when necessary during such operation.
        m_myWindow.pollEvents();
		
        // Please note that commandBuffer could be null pointer
        // if the swapChain needs to be recreated
        if (auto commandBuffer = m_myRenderer.beginFrame())
		{
            // In case we have multiple render passes for the current frame
            // begin offsreen shadow pass
            // render shadow casting objects
            // end offscreen shadow pass

            m_myRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderFactotry.renderGameObjects(commandBuffer, m_vMyGameObjects);
            m_myRenderer.endSwapChainRenderPass(commandBuffer);

            m_myRenderer.endFrame();
        }
    }

    // GPU will block until all CPU is complete
    vkDeviceWaitIdle(m_myDevice.device());
}

void MyApplication::_loadGameObjects()
{
    // add postition and color interleaved vertex buffer
    std::vector<MyModel::Vertex> vertices
	{
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

    auto model = std::make_shared<MyModel>(m_myDevice, vertices);

    auto triangle = MyGameObject::createGameObject();
    triangle.model = model;
    triangle.color = { 0.1f, 0.8f, 0.1f };
    triangle.transform2d.translation.x = 0.8f;
    triangle.transform2d.scale = {1.0f, 1.0f};
    triangle.transform2d.rotation = 0.125f * glm::two_pi<float>(); // 90 degree rotation

    m_vMyGameObjects.push_back(std::move(triangle));
}

