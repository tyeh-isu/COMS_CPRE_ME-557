#include "my_application.h"

// Render factory
#include "my_simple_render_factory.h"
#include "my_camera.h"

// use radian rather degree for angle
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// Std
#include <stdexcept>
#include <array>

MyApplication::MyApplication() :
    m_bPerspectiveProjection(true)
{
    _loadGameObjects();
}

void MyApplication::run() 
{
    static bool bPerspectiveCamera = true;

    m_myWindow.bindMyApplication(this);
    MySimpleRenderFactory simpleRenderFactotry{ m_myDevice, m_myRenderer.swapChainRenderPass() };
    MyCamera camera{};

    while (!m_myWindow.shouldClose()) 
    {
        // Note: depending on the platform (Windows, Linux or Mac), this function
        // will cause the event proecssing to block during a Window move, resize or
        // menu operation. Users can use the "window refresh callback" to redraw the
        // contents of the window when necessary during such operation.
        m_myWindow.pollEvents();

        float apsectRatio = m_myRenderer.aspectRatio();

        // Put it here because the viewport may change
        if (m_bPerspectiveProjection)
            // near and far will automatically apply negative values
            camera.setPerspectiveProjection(glm::radians(50.f), apsectRatio, 0.1f, 10.f);
        else
            // because Y is down by default for Vulkan, when we set top to be minus value, we can flip the coordinate
            // such that Y is up. Because we move the part 2.5 units, the near and far value needs to cover the model
            // Also, near and far will automatically apply negative values
            camera.setOrthographicProjection(-apsectRatio, apsectRatio, -1.0f, 1.0f, -3.0f, 3.0f); 
 
        // Please note that commandBuffer could be null pointer
        // if the swapChain needs to be recreated
        if (auto commandBuffer = m_myRenderer.beginFrame())
		{
            // In case we have multiple render passes for the current frame
            // begin offsreen shadow pass
            // render shadow casting objects
            // end offscreen shadow pass

            m_myRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderFactotry.renderGameObjects(commandBuffer, m_vMyGameObjects, camera);
            m_myRenderer.endSwapChainRenderPass(commandBuffer);

            m_myRenderer.endFrame();
        }
    }

    // GPU will block until all CPU is complete
    vkDeviceWaitIdle(m_myDevice.device());
}

void MyApplication::switchProjectionMatrix()
{
    // Switch between perspective and orthographic projection matrix
    m_bPerspectiveProjection = !m_bPerspectiveProjection;
}

std::unique_ptr<MyModel> createCubeModel(MyDevice& device, glm::vec3 offset)
{
    // Unit cube
    std::vector<MyModel::Vertex> vertices
    {
       // left face (white)
      {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
      {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
      {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},

      // right face (yellow)
      {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
      {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
      {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .8f, .1f}},

      // top face (orange, remember y axis points down)
      {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
      {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
      {{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
      {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
      {{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
      {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},

      // bottom face (red)
      {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
      {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
      {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .1f, .1f}},

      // front face (blue)
      {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
      {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
      {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},

      // back face (green)
      {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
      {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
      {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
    };

    for (auto& v : vertices)
    {
        v.position += offset;
    }

    return std::make_unique<MyModel>(device, vertices);
}

void MyApplication::_loadGameObjects()
{
    std::shared_ptr<MyModel> myModel = createCubeModel(m_myDevice, { .0f, .0f, .0f });
    
    // Note: +X to the right, +Y down and +Z inside the screen
    auto cube = MyGameObject::createGameObject();
    cube.model = myModel;

    //
    // Please note that the view position right now is at (0,0,0)
    // if we use OpenGL projection matrix (Y is up and Z is out of the screen), the object needs to be in -Z direction in order to see
    // the object
    // if we use Vulkan tradition (Y is down and Z is into the screen), the obejct needs to be in +Z direction
    //
    cube.transform.translation = { 0.0f, 0.0f, -2.5f }; // for perspective
    cube.transform.scale = { 1.0f, 1.0f, 1.0f };

    m_vMyGameObjects.push_back(std::move(cube));
}

