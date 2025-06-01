#include "my_application.h"

// Render factory
#include "my_simple_render_factory.h"
#include "my_camera.h"
#include "my_keyboard_controller.h"

// use radian rather degree for angle
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// Std
#include <stdexcept>
#include <array>
#include <chrono>

MyApplication::MyApplication() :
    m_bPerspectiveProjection(true)
{
    _loadGameObjects();
}

void MyApplication::run() 
{
    m_myWindow.bindMyApplication(this);
    MySimpleRenderFactory simpleRenderFactotry{ m_myDevice, m_myRenderer.swapChainRenderPass() };
    MyCamera camera{};

    // Empty object
    auto viewerObject = MyGameObject::createGameObject();
    MyKeyboardController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!m_myWindow.shouldClose()) 
    {
        // Note: depending on the platform (Windows, Linux or Mac), this function
        // will cause the event proecssing to block during a Window move, resize or
        // menu operation. Users can use the "window refresh callback" to redraw the
        // contents of the window when necessary during such operation.
        m_myWindow.pollEvents();

        // Need to get the call after glfwPollEvants because the call above may take time
        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        cameraController.moveInPlaneXZ(m_myWindow.glfwWindow(), frameTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        float apsectRatio = m_myRenderer.aspectRatio();

        // Put it here because the viewport may change
        if (m_bPerspectiveProjection)
            // near and far will automatically apply negative values
            camera.setPerspectiveProjection(glm::radians(50.f), apsectRatio, 0.1f, 10.f);
        else
            // because Y is down by default for Vulkan, when we set top to be minus value, we can flip the coordinate
            // such that Y is up. Because we move the part 2.5 units, the near and far value needs to cover the model
            // Also, near and far will automatically apply negative values
            camera.setOrthographicProjection(-apsectRatio * 2.0f, apsectRatio * 2.0f, -2.0f, 2.0f, -5.0f, 5.0f);
 
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

void MyApplication::_loadGameObjects()
{
    std::shared_ptr<MyModel> mymodel =
        MyModel::createModelFromFile(m_myDevice, "models/smooth_vase.obj");

    // Note: +X to the right, +Y down and +Z inside the screen
    auto mygameobj = MyGameObject::createGameObject();
    mygameobj.model = mymodel;
    
    //cube.transform.translation = { .0f, .0f, 0.5f }; // for orthographic
    mygameobj.transform.translation = { .0f, .0f, -2.5f }; // for perspective

    mygameobj.transform.scale = glm::vec3(2.0f);
	mygameobj.transform.rotation.x = glm::pi<float>(); // rotate 180 so Y is up
    m_vMyGameObjects.push_back(std::move(mygameobj));
}

