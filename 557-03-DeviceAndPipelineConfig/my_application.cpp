#include "my_application.h"

MyApplication::MyApplication()
{
    _createPipeline();
}

void MyApplication::run() 
{
    while (!m_myWindow.shouldClose()) 
    {
        glfwPollEvents();
    }
}

void MyApplication::_createPipeline()
{
    // Note: the swap chain buffer size may not be the same as the Window size
    // Pipeline config can be considered as a blue print to create graphics pipeline
    PipelineConfigInfo pipelineConfig{};

    MyPipeline::defaultPipelineConfigInfo(pipelineConfig, WIDTH, HEIGHT);

    // For now render pass contains the structure and the format
    // of the frame buffer object and its attachments
    // for example in the defect FBO, attachment 0 is the color buffer
    // and attachment 1 is the depth buffer
    pipelineConfig.renderPass = nullptr;
    pipelineConfig.pipelineLayout = nullptr;

    m_pMyPipeline = std::make_unique< MyPipeline >(
        m_myDevice,
        "shaders/simple_shader.vert.spv",
        "shaders/simple_shader.frag.spv",
        pipelineConfig
        );
}

