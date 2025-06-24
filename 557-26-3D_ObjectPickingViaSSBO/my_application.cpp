#include "my_application.h"

// Render factories
#include "my_simple_render_factory.h"
#include "my_pointlight_render_factory.h"
#include "my_picking_factory.h"
#include "my_camera.h"
#include "my_keyboard_controller.h"
#include "my_buffer.h"

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

// Move MyGlobalUBO to my_frame_info.h

struct MySSBO
{
    float id;
};

MyApplication::MyApplication() :
    m_bPerspectiveProjection(true),
    m_fPickID(0.0f)
{
    m_pMyGlobalPool =
        MyDescriptorPool::Builder(m_myDevice)
        .setMaxSets(MySwapChain::MAX_FRAMES_IN_FLIGHT) // for descriptor set
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MySwapChain::MAX_FRAMES_IN_FLIGHT)
		.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1) // for picking
        .build();

    _loadGameObjects();

    m_fMousePos[0] = -1.0f;
    m_fMousePos[1] = -1.0f;
}

void MyApplication::run() 
{
	m_myGUIData.init();

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

    // Create SSBO for picking
    std::unique_ptr<MyBuffer> ssboBuffer;
    ssboBuffer = std::make_unique<MyBuffer>(
        m_myDevice,
        sizeof(MySSBO),
        1,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, // no host coherience bit but we could do it here, so we don't need to use flush. We need to revisit here and see if we can remove the last aurgment
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    ssboBuffer->map();

    // Create descriptor set layout object
    auto globalSetLayout =
        MyDescriptorSetLayout::Builder(m_myDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS) // can be accessed all shader stages
		.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // can be accessed by fragment shader
        .build();

    // Create one descriptor set per frame
    std::vector<VkDescriptorSet> globalDescriptorSets(MySwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < globalDescriptorSets.size(); i++)
	{
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        auto ssbobufferInfo = ssboBuffer->descriptorInfo();

        MyDescriptorWriter(*globalSetLayout, *m_pMyGlobalPool)
            .writeBuffer(0, &bufferInfo)      // ubo bind to 0
            .writeBuffer(1, &ssbobufferInfo)  // ssbo bind to 1
            .build(globalDescriptorSets[i]);
    }

    MySimpleRenderFactory simpleRenderFactory
    {
        m_myDevice,
        m_myRenderer.swapChainRenderPass(),
        globalSetLayout->descriptorSetLayout() 
    };

    MyPointLightRenderFactory pointLightFactory
    {
        m_myDevice,
        m_myRenderer.swapChainRenderPass(),
        globalSetLayout->descriptorSetLayout()
    };

    MyPickingFactory pickingFactory
	{ 
	    m_myDevice,
		m_myRenderer,
		globalSetLayout->descriptorSetLayout()
	};

    // Initial the picking SSBO buffer
    MySSBO ssbo{};
    ssbo.id = 0.0f;
    ssboBuffer->writeToBuffer(&ssbo);
    ssboBuffer->unmap();

    m_myWindow.bindMyApplication(this);
    MyCamera camera{};
    MyKeyboardController cameraController{};

    // Empty object to store camera transformation matrix
    auto viewerObject = MyGameObject::createGameObject();
    viewerObject.transform.translation.z = 3.5f; // move the camera back so the light object can be seen

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
              globalDescriptorSets[frameIndex],
              m_mapGameObjects
            };

            // update UBO to GPU
            MyGlobalUBO ubo{};
            ubo.projection = camera.projectionMatrix();
            ubo.view = camera.viewMatrix();
            ubo.inverseView = camera.inverseViewMatrix();

            // update light color from GUI
            ubo.pointLight.color.r = m_myGUIData.vColor.x;
            ubo.pointLight.color.g = m_myGUIData.vColor.y;
            ubo.pointLight.color.b = m_myGUIData.vColor.z;

            // update light position from GUI
            ubo.pointLight.position.x += (m_myGUIData.fMoveValues[0] - 0.5f) * 2.0f;
            ubo.pointLight.position.y += (m_myGUIData.fMoveValues[1] - 0.5f) * 2.0f;
            ubo.pointLight.position.z += (m_myGUIData.fMoveValues[2] - 0.5f) * 2.0f;

            // picking
            ubo.pickedObjectID = static_cast<unsigned int>(m_fPickID);

            // update light position from keyboard (K & L)
			pointLightFactory.update(ubo, m_v3LightOffset);

            uboBuffers[frameIndex]->writeToBuffer(&ubo);

            // becasue we don't use host coherence flag, we need to call flash
            uboBuffers[frameIndex]->flush();

            // Picking
			if (m_myGUIData.bPickMode && m_fMousePos[0] >= 0.0f && m_fMousePos[1] >= 0.0f)
			{
			    MySSBO ssbo{};
                ssbo.id = 0.0f;
                ssboBuffer->map();
                ssboBuffer->writeToBuffer(&ssbo);
                ssboBuffer->unmap();

                m_myRenderer.beginPickRenderPass(commandBuffer, (int)m_fMousePos[0], (int)m_fMousePos[1], false);

                pickingFactory.renderPickScene(frameInfo);
                m_myRenderer.endPickRenderPass(commandBuffer);
			}
			else // Normal rendering
			{
                // render
                m_myRenderer.beginSwapChainRenderPass(commandBuffer);

                // render GUI
                m_myGUI.draw(commandBuffer, m_myGUIData);

                // render game objects
                simpleRenderFactory.renderGameObjects(frameInfo);

                // render light
                if (m_myGUIData.bShowLight)
                    pointLightFactory.render(frameInfo);

                m_myRenderer.endSwapChainRenderPass(commandBuffer);
            }

            m_myRenderer.endFrame();

            // After the rendering, get the result from SSBO
            if (m_myGUIData.bPickMode)
            {
                MySSBO ssbo{};
                ssbo.id = 0.0f;
                ssboBuffer->map();
                ssboBuffer->readFromBuffer(&ssbo);
                ssboBuffer->unmap();
                //std::cout << "pick id = " << ssbo.id << std::endl;
                m_fPickID = ssbo.id;

                if (m_fPickID == 0)
                    m_myGUIData.sPickObject = "None";
                else if (m_fPickID == 100)
                    m_myGUIData.sPickObject = "Flat Vase";
                else if (m_fPickID == 200)
                    m_myGUIData.sPickObject = "Smooth Vase";
                else if (m_fPickID == 300)
                    m_myGUIData.sPickObject = "Floor";
            }
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

void MyApplication::_loadGameObjects()
{
    std::shared_ptr<MyModel> mymodel1 =
        MyModel::createModelFromFile(m_myDevice, "models/flat_vase.obj", 100);

    // Note: +X to the right, +Y down and +Z inside the screen
    auto flatVase = MyGameObject::createGameObject();
    flatVase.model = mymodel1;

    flatVase.transform.translation = { -.5f, -.5f, 0.0f };
    flatVase.transform.scale = { 3.f, 1.5f, 3.f };
    flatVase.transform.rotation.x = glm::pi<float>(); // rotate 180 so Y is up
    m_mapGameObjects.emplace(flatVase.getID(), std::move(flatVase));

    // Load a second model
    std::shared_ptr<MyModel> mymodel2 = MyModel::createModelFromFile(m_myDevice, "models/smooth_vase.obj", 200);
    auto smoothVase = MyGameObject::createGameObject();
    smoothVase.model = mymodel2;
    smoothVase.transform.translation = { .5f, -.5f, 0.0f };
    smoothVase.transform.scale = { 3.f, 1.5f, 3.f };
    smoothVase.transform.rotation.x = glm::pi<float>(); // rotate 180 so Y is up
    m_mapGameObjects.emplace(smoothVase.getID(), std::move(smoothVase));

    // Load a quad model
    std::shared_ptr<MyModel> mymodel3 = MyModel::createModelFromFile(m_myDevice, "models/quad.obj", 300);
    auto floor = MyGameObject::createGameObject();
    floor.model = mymodel3;
    floor.transform.translation = { 0.f, -.5f, 0.f };
    floor.transform.scale = { 3.f, 1.f, 3.f };
	floor.transform.rotation.x = glm::pi<float>(); // rotate 180 so Y is up
    m_mapGameObjects.emplace(floor.getID(), std::move(floor));
}

void MyApplication::togglePickMode()
{
    m_myGUIData.bPickMode = !m_myGUIData.bPickMode;

    if (m_myGUIData.bPickMode)
        std::cout << "Pick Mode" << std::endl;
    else
        std::cout << "Render Mode" << std::endl;
}

void MyApplication::mouseButtonEvent(bool bMouseDown, float posx, float posy)
{
    if (bMouseDown)
    {
        m_fMousePos[0] = posx;
        m_fMousePos[1] = posy;
    }
    else
    {
        m_fMousePos[0] = -1.0f;
        m_fMousePos[1] = -1.0f;
    }
}

