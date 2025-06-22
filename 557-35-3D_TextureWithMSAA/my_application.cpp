#include "my_application.h"

// Render factories
#include "my_texture_render_factory.h"
#include "my_pointlight_render_factory.h"
#include "my_camera.h"
#include "my_keyboard_controller.h"
#include "my_buffer.h"
#include "my_texture.h"

// use radian rather degree for angle
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// Std
#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>
#include <numeric>
#include <iostream>

const std::string MODEL_PATH_1 = "./models/viking_room.obj";
const std::string MODEL_PATH_2 = "./models/quad.obj";
const std::string TEXTURE_PATH_1 = "./textures/viking_room.png";
const std::string TEXTURE_PATH_2 = "./textures/checkerboard-pattern.jpg";

MyApplication::MyApplication() :
    m_bPerspectiveProjection(true),
    m_bSupportMSAA(true),
    m_bUpdateForMSAA(false)
{
    m_pMyGlobalPool =
        MyDescriptorPool::Builder(m_myDevice)
        .setMaxSets(MySwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MySwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MySwapChain::MAX_FRAMES_IN_FLIGHT)
        .build();

    _loadGameObjects();
}

void MyApplication::run() 
{
    // Since texture image don't change every frame, only create one
	MyTexture myTexture1(m_myDevice, TEXTURE_PATH_1);
	MyTexture myTexture2(m_myDevice, TEXTURE_PATH_2);

	VkDescriptorImageInfo textureDescriptorImageInfos[2];
	textureDescriptorImageInfos[0] = myTexture1.descriptorInfo();
	textureDescriptorImageInfos[1] = myTexture2.descriptorInfo();

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
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)  // Unfirom buffer can be accessed all shader stages
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2) // Texture image can only be accessed by fragment shader stage
        .build();

    // Create one descriptor set per frame
    std::vector<VkDescriptorSet> globalDescriptorSets(MySwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++)
	{
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        MyDescriptorWriter(*globalSetLayout, *m_pMyGlobalPool)
            .writeBuffer(0, &bufferInfo)
            .writeImages(1, textureDescriptorImageInfos, 2)
            .build(globalDescriptorSets[i]);
    }

    MyPointLightRenderFactory pointLightFactory
    {
        m_myDevice,
        m_myRenderer.swapChainRenderPass(),
        globalSetLayout->descriptorSetLayout() 
    };

    MyTextureRenderFactory textureFactory
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
    viewerObject.transform.translation.x = 0.0f;
    viewerObject.transform.translation.y = 0.5f;
    viewerObject.transform.translation.z = 2.5f;

    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!m_myWindow.shouldClose()) 
    {
        // Note: depending on the platform (Windows, Linux or Mac), this function
        // will cause the event proecssing to block during a Window move, resize or
        // menu operation. Users can use the "window refresh callback" to redraw the
        // contents of the window when necessary during such operation.
        m_myWindow.pollEvents();

        // Check if we need to update MSAA
        if (m_bUpdateForMSAA)
        {
            m_bUpdateForMSAA = false;

            if (m_bSupportMSAA)
            {
                std::cout << "***** Turn on multiple sampling *****" << std::endl;
                m_myDevice.setMsaaSamples(true);
            }
            else
            {
                std::cout << "***** Turn off multiple sampling *****" << std::endl;
                m_myDevice.setMsaaSamples(false);
            }

			// Since we change the number of samples, need to recrate swapchain, renderer and 
            m_myRenderer.recreateSwapChain();

            textureFactory.recratePipeline(m_myRenderer.swapChainRenderPass());
            pointLightFactory.recratePipeline(m_myRenderer.swapChainRenderPass());
        }

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
              globalDescriptorSets[frameIndex],
              m_mapGameObjects
            };

            // update UBO to GPU
            MyGlobalUBO ubo{};
            ubo.projection = camera.projectionMatrix();
            ubo.view = camera.viewMatrix();
            ubo.inverseView = camera.inverseViewMatrix();

            // update light position from keyboard (K & L)
			pointLightFactory.update(ubo, m_v3LightOffset);

            uboBuffers[frameIndex]->writeToBuffer(&ubo);

            // becasue we don't use host coherence flag, we need to call flash
            uboBuffers[frameIndex]->flush();

            // render
            m_myRenderer.beginSwapChainRenderPass(commandBuffer);

            pointLightFactory.render(frameInfo);
            textureFactory.render(frameInfo);

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

void MyApplication::updateLightPosition(glm::vec3 updateLightPos)
{
    m_v3LightOffset += updateLightPos;
}

void MyApplication::updateMSAA()
{
    m_bSupportMSAA = !m_bSupportMSAA;
    m_bUpdateForMSAA = true;
}

void MyApplication::_loadGameObjects()
{
    std::shared_ptr<MyModel> mymodel1 =
        MyModel::createModelFromFile(m_myDevice, MODEL_PATH_1);

    // Note: +X to the right, +Y down and +Z inside the screen
    auto viking_room = MyGameObject::createGameObject();
    viking_room.textureModel = mymodel1;

    viking_room.transform.translation = { 0.f, 0.0f, 0.f };
    viking_room.transform.scale = { 1.0f, 1.0f, 1.0f };
    viking_room.transform.rotation.x = -glm::pi<float>() / 2.0f; // rotate 90
    viking_room.transform.rotation.y = glm::pi<float>(); // rotate 180

    m_mapGameObjects.emplace(viking_room.getID(), std::move(viking_room));

    std::shared_ptr<MyModel> mymodel2 =
        MyModel::createModelFromFile(m_myDevice, MODEL_PATH_2);
	auto floor = MyGameObject::createGameObject();
    floor.textureModel = mymodel2;

    floor.transform.translation = { 0.f, 0.0f, 0.f };
    floor.transform.scale = { 5.f, 1.f, 5.f };
    floor.transform.rotation.x = glm::pi<float>(); // rotate 180

    m_mapGameObjects.emplace(floor.getID(), std::move(floor));
}

