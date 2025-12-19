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
#include <numeric>

struct MyGlobalUBO 
{
    alignas(16) glm::mat4 projectionView{ 1.f };
    alignas(16) glm::vec3 lightDirection = glm::normalize(glm::vec3{ -1.f, 3.f, 1.f });

    // and we can add more here...
};

MyApplication::MyApplication() :
    m_bPerspectiveProjection(true)
{
    m_pMyGlobalPool =
        MyDescriptorPool::Builder(m_myDevice)
        .setMaxSets(MySwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MySwapChain::MAX_FRAMES_IN_FLIGHT)
        .build();

    _loadGameObjects();
}

void MyApplication::run() 
{
    // Create buffer for each frame to avoid memory accessing issue
    // Note: with the new map function, we don't need to worry about
    // offset alignment for UBO buffer
    std::vector<std::unique_ptr<MyBuffer>> uboBuffers(MySwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < uboBuffers.size(); i++) 
    {
        uboBuffers[i] = std::make_unique<MyBuffer>(
            m_myDevice,
            sizeof(MyGlobalUBO),
            1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT); // no host coherience bit but we could do it here, so we don't need to use flush. We need to revisit here and see if we can remove the last aurgment

        uboBuffers[i]->map();
    }

    // Create descriptor set layout object
    auto globalSetLayout =
        MyDescriptorSetLayout::Builder(m_myDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)  // can be accessed by vertex shader
        .build();

    // Create one descriptor set per frame
    std::vector<VkDescriptorSet> globalDescriptorSets(MySwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++)
	{
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        MyDescriptorWriter(*globalSetLayout, *m_pMyGlobalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }

    MySimpleRenderFactory simpleRenderFactory
    {
        m_myDevice,
        m_myRenderer.swapChainRenderPass(),
        globalSetLayout->descriptorSetLayout()
    };

    m_myWindow.bindMyApplication(this);
    MyCamera camera{};
    MyKeyboardController cameraController{};

    // Empty object to store camera transformation matrix
    auto viewerObject = MyGameObject::createGameObject();

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

            int frameIndex = m_myRenderer.frameIndex();
            MyFrameInfo frameInfo
            {
              frameIndex,
              frameTime,
              commandBuffer,
              camera,
              globalDescriptorSets[frameIndex]
            };

            // update UBO to GPU
            MyGlobalUBO ubo{};
            ubo.projectionView = camera.projectionMatrix() * camera.viewMatrix();
            uboBuffers[frameIndex]->writeToBuffer(&ubo);

            // becasue we don't use host coherence flag, we need to call flash
            uboBuffers[frameIndex]->flush();

            // render
            m_myRenderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderFactory.renderGameObjects(frameInfo, m_vMyGameObjects);
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
        MyModel::createModelFromFile(m_myDevice, "models/flat_vase.obj");

    // Note: +X to the right, +Y down and +Z inside the screen
    auto flatVase = MyGameObject::createGameObject();
    flatVase.model = mymodel;

    flatVase.transform.translation = { -.5f, -.5f, -2.5f };
    flatVase.transform.scale = { 3.f, 1.5f, 3.f };
    flatVase.transform.rotation.x = glm::pi<float>(); // rotate 180 so Y is up
    m_vMyGameObjects.push_back(std::move(flatVase));

    // Load a second model
    mymodel = MyModel::createModelFromFile(m_myDevice, "models/smooth_vase.obj");
    auto smoothVase = MyGameObject::createGameObject();
    smoothVase.model = mymodel;
    smoothVase.transform.translation = { .5f, -.5f, -2.5f };
    smoothVase.transform.scale = { 3.f, 1.5f, 3.f };
    smoothVase.transform.rotation.x = glm::pi<float>(); // rotate 180 so Y is up
    m_vMyGameObjects.push_back(std::move(smoothVase));
}

