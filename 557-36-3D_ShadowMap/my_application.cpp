#include "my_application.h"

// Render factories
#include "my_simple_render_factory.h"
#include "my_texture_render_factory.h"
#include "my_pointlight_render_factory.h"
#include "my_picking_factory.h"
#include "my_offscreen_render_factory.h"
#include "my_debug_render_factory.h"
#include "my_camera.h"
#include "my_keyboard_controller.h"
#include "my_buffer.h"
#include "my_texture.h"

// use radian rather degree for angle
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

// Std
#include <stdexcept>
#include <array>
#include <cassert>
#include <chrono>
#include <numeric>
#include <iostream>

const std::string MODEL_PATH_1 = "./models/viking_room.obj";
const std::string MODEL_PATH_2 = "./models/quad.obj";
const std::string MODEL_PATH_3 = "models/smooth_vase.obj";

const std::string TEXTURE_PATH_1 = "./textures/viking_room.png";
const std::string TEXTURE_PATH_2 = "./textures/checkerboard-pattern.jpg";

// Move MyGlobalUBO to my_frame_info.h
#define RENDER_SHADOW 1

MyApplication::MyApplication() :
    m_bPerspectiveProjection(true),
    m_fPickID(0.0f)
{
    // Pool for normal rendering
    m_pMyGlobalPool =
        MyDescriptorPool::Builder(m_myDevice)
        .setMaxSets(MySwapChain::MAX_FRAMES_IN_FLIGHT+3) // for descriptor set
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MySwapChain::MAX_FRAMES_IN_FLIGHT) // for normal rendering
		.addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1) // for picking
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MySwapChain::MAX_FRAMES_IN_FLIGHT) // for texure and shadow map
		.build();

    // Pool for offscreen rendering
    m_pMyOffscreenPool =
        MyDescriptorPool::Builder(m_myDevice)
        .setMaxSets(MySwapChain::MAX_FRAMES_IN_FLIGHT) // for descriptor set
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MySwapChain::MAX_FRAMES_IN_FLIGHT) // for shadow map rendering
        .build();

    _loadGameObjects();

    m_fMousePos[0] = -1.0f;
    m_fMousePos[1] = -1.0f;
}

void MyApplication::run() 
{
    // Since texture image don't change every frame, only create one
	MyTexture myTexture1(m_myDevice, TEXTURE_PATH_1);
	MyTexture myTexture2(m_myDevice, TEXTURE_PATH_2);

#if RENDER_SHADOW
	VkDescriptorImageInfo textureDescriptorImageInfos[3];
	textureDescriptorImageInfos[0] = myTexture1.descriptorInfo();
	textureDescriptorImageInfos[1] = myTexture2.descriptorInfo();
    textureDescriptorImageInfos[2] = m_myRenderer.shadowMapDescriptorInfo();
#else
    VkDescriptorImageInfo textureDescriptorImageInfos[3];
    textureDescriptorImageInfos[0] = myTexture1.descriptorInfo();
    textureDescriptorImageInfos[1] = myTexture2.descriptorInfo();
    textureDescriptorImageInfos[2] = m_myRenderer.shadowMapDescriptorInfo();
#endif

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
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); // no host coherience bit but we could do it here, so we don't need to use flush. We need to revisit here and see if we can remove the last aurgment

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

    // Create descriptor set layout object for scene rendering
    auto globalSetLayout =
        MyDescriptorSetLayout::Builder(m_myDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS) // can be accessed all shader stages
		.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // can be accessed by fragment shader
#if RENDER_SHADOW
		.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3) // can be accessed by fragment shader for shadow map
#else
        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2) // can be accessed by fragment shader for shadow map
#endif
        .build();

    // Create decriptor set layout object for shadow map
    auto offscreenSetLayout =
        MyDescriptorSetLayout::Builder(m_myDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS) // can be accessed all shader stages
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
#if RENDER_SHADOW
            .writeImages(2, textureDescriptorImageInfos, 3) // only need to write the first two textures
#else
            .writeImages(2, textureDescriptorImageInfos, 2) // only need to write the first two textures
#endif
            .build(globalDescriptorSets[i]);
    }

    // Create one descriptor set per frame
    std::vector<VkDescriptorSet> offscreenDescriptorSets(MySwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < offscreenDescriptorSets.size(); i++)
    {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();

        MyDescriptorWriter(*offscreenSetLayout, *m_pMyOffscreenPool)
            .writeBuffer(0, &bufferInfo)      // ubo bind to 0
            .build(offscreenDescriptorSets[i]);
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

    MyDebugRenderFactory debugFactory
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

    MyOffScreenRenderFactory offscreenRenderFactory
    {
        m_myDevice,
        m_myRenderer.offscreenRenderPass(),
        offscreenSetLayout->descriptorSetLayout()
    };

    MyTextureRenderFactory textureFactory
    {
        m_myDevice,
        m_myRenderer.swapChainRenderPass(),
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
    viewerObject.transform.translation.x = 0.0f;
    viewerObject.transform.translation.y = 0.5f;
    viewerObject.transform.translation.z = 3.5f;

    auto currentTime = std::chrono::high_resolution_clock::now();

    int resize = 0;

    while (!m_myWindow.shouldClose()) 
    {
        if (resize)
        {
            for (int i = 0; i < uboBuffers.size(); i++)
            {
                uboBuffers[i]->unmap();
            }

            globalDescriptorSets.clear();
            globalDescriptorSets.shrink_to_fit();
            globalDescriptorSets.resize(MySwapChain::MAX_FRAMES_IN_FLIGHT);

            VkDescriptorImageInfo updateTextureDescriptorImageInfos[3];
            updateTextureDescriptorImageInfos[0] = myTexture1.descriptorInfo();
            updateTextureDescriptorImageInfos[1] = myTexture2.descriptorInfo();
            updateTextureDescriptorImageInfos[2] = m_myRenderer.shadowMapDescriptorInfo();

            for (int i = 0; i < uboBuffers.size(); i++)
            {
                uboBuffers[i]->map();
            }

            for (int i = 0; i < globalDescriptorSets.size(); i++)
            {
                auto bufferInfo = uboBuffers[i]->descriptorInfo();
                auto ssbobufferInfo = ssboBuffer->descriptorInfo();

                MyDescriptorWriter(*globalSetLayout, *m_pMyGlobalPool)
                    .writeBuffer(0, &bufferInfo)      // ubo bind to 0
                    .writeBuffer(1, &ssbobufferInfo)  // ssbo bind to 1
#if RENDER_SHADOW
                    .writeImages(2, updateTextureDescriptorImageInfos, 3) // only need to write the first two textures
#else
                    .writeImages(2, updateTextureDescriptorImageInfos, 2) // only need to write the first two textures
#endif
                    .build(globalDescriptorSets[i]);
            }

            // Create one descriptor set per frame
            /*offscreenDescriptorSets.clear();
            offscreenDescriptorSets.shrink_to_fit();
            offscreenDescriptorSets.resize(MySwapChain::MAX_FRAMES_IN_FLIGHT);

            for (int i = 0; i < offscreenDescriptorSets.size(); i++)
            {
                auto bufferInfo = uboBuffers[i]->descriptorInfo();

                MyDescriptorWriter(*offscreenSetLayout, *m_pMyOffscreenPool)
                    .writeBuffer(0, &bufferInfo)      // ubo bind to 0
                    .build(offscreenDescriptorSets[i]);
            }*/

            simpleRenderFactory.recratePipeline(m_myRenderer.swapChainRenderPass());
            pointLightFactory.recratePipeline(m_myRenderer.swapChainRenderPass());
            debugFactory.recratePipeline(m_myRenderer.swapChainRenderPass());
            pickingFactory.recratePipeline(m_myRenderer);
            offscreenRenderFactory.recratePipeline(m_myRenderer.offscreenRenderPass());
            textureFactory.recratePipeline(m_myRenderer.swapChainRenderPass());

            /*for (int i = 0; i < uboBuffers.size(); i++)
            {
                uboBuffers[i]->map();
            }*/

            resize = 0;
        }

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
            camera.setPerspectiveProjection(glm::radians(45.f), apsectRatio, 0.1f, 96.f);
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
              offscreenDescriptorSets[frameIndex],
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

            // Light ModelViewProjection matrix
            MyCamera lightCamera{};
            lightCamera.setPerspectiveProjection(glm::radians(70.f), 1.0, 0.1f, 96.f);

            glm::vec3 lightPos;
            lightPos.x = (ubo.pointLight.position.x + m_v3LightOffset.x);
            lightPos.y = (ubo.pointLight.position.y + m_v3LightOffset.y);
            lightPos.z = (ubo.pointLight.position.z + m_v3LightOffset.z);

            //= glm::vec3(ubo.pointLight.position) + m_v3LightOffset;
            //lightCamera.setViewTarget(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));

            //lightCamera.setViewYXZ(lightPos, glm::vec3(0.0f, 0.0f, 0.0f));

            glm::mat4 viewm = lightCamera.viewMatrix();

            //viewm = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0, 1, 0));
            //viewm = glm::inverse(viewm);

            glm::mat4 lightProjectionMatrix = lightCamera.projectionMatrix(); //glm::perspective((float)glm::radians(45.0f), 1.0f, 0.1f, 10.f);
            glm::mat4 lightViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
            glm::mat4 lightModelMatrix = glm::mat4(1.0f);
            ubo.pointLight.lightMVP = lightProjectionMatrix * lightViewMatrix * lightModelMatrix;

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
                // First render shadow map
                m_myRenderer.beginOffscreenRenderPass(commandBuffer);
                offscreenRenderFactory.renderGameObjects(frameInfo);
                m_myRenderer.endOffscreenRenderPass(commandBuffer);

                // render normal scene
                m_myRenderer.beginSwapChainRenderPass(commandBuffer);

                // render GUI
                m_myGUI.draw(commandBuffer, m_myGUIData);

                // render game objects
                if (m_myGUIData.bShowDebug)
                {
                    debugFactory.renderGameObjects(frameInfo);
                }
                else
                {
                    textureFactory.render(frameInfo);
					simpleRenderFactory.render(frameInfo);
                }

                // render light
                if (m_myGUIData.bShowLight)
                    pointLightFactory.render(frameInfo);

                m_myRenderer.endSwapChainRenderPass(commandBuffer);
            }

            resize = m_myRenderer.endFrame();
            if (resize == 1)
            {
                //vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
            }

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
                    m_myGUIData.sPickObject = "Viking Room";
                else if (m_fPickID == 200)
                    m_myGUIData.sPickObject = "Floor";
                else if (m_fPickID == 300)
                    m_myGUIData.sPickObject = "Smooth Vase";
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
        MyModel::createModelFromFile(m_myDevice, MODEL_PATH_1, 100);

    // Note: +X to the right, +Y down and +Z inside the screen

    // Load first texture model
    auto viking_room = MyGameObject::createGameObject(MyGameObject::TEXTURE);
    viking_room.model = mymodel1;

    viking_room.transform.translation = { 0.f, 0.0f, 0.f };
    viking_room.transform.scale = { 1.0f, 1.0f, 1.0f };
    viking_room.transform.rotation.x = -glm::pi<float>() / 2.0f; // rotate 90
    viking_room.transform.rotation.y = glm::pi<float>(); // rotate 180

    m_mapGameObjects.emplace(viking_room.getID(), std::move(viking_room));

    // Load second texture model - floor
    // Please note that MyTextureRenderFactory::render assume first texture applies to model1
    // and the second texture applies to model2
    std::shared_ptr<MyModel> mymodel3 =
        MyModel::createModelFromFile(m_myDevice, MODEL_PATH_2, 200);

    auto floor = MyGameObject::createGameObject(MyGameObject::TEXTURE);
    floor.model = mymodel3;

    floor.transform.translation = { 0.f, 0.0f, 0.f };
    floor.transform.scale = { 5.f, 1.f, 5.f };
    floor.transform.rotation.x = glm::pi<float>(); // rotate 180
    m_mapGameObjects.emplace(floor.getID(), std::move(floor));

    // Load a third simple model
    std::shared_ptr<MyModel> mymodel2 = MyModel::createModelFromFile(m_myDevice, MODEL_PATH_3, 300);
    auto smoothVase = MyGameObject::createGameObject(MyGameObject::SIMPLE);
    smoothVase.model = mymodel2;
    smoothVase.transform.translation = { -0.6f, 0.1f, 0.5f };
    smoothVase.transform.scale = { 1.5f, 0.75f, 1.5f };
    smoothVase.transform.rotation.x = glm::pi<float>(); // rotate 180 so Y is up
    m_mapGameObjects.emplace(smoothVase.getID(), std::move(smoothVase));

    // Create debug model
	auto debugfloor = MyGameObject::createGameObject(MyGameObject::DEBUG);
    debugfloor.model = mymodel3;

    debugfloor.transform.translation = { 0.f, 0.0f, 0.f };
    debugfloor.transform.scale = { 5.f, 1.f, 5.f };
    debugfloor.transform.rotation.x = glm::pi<float>(); // rotate 180
    m_mapGameObjects.emplace(debugfloor.getID(), std::move(debugfloor));
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

