#ifndef __MY_RENDERER_H__
#define __MY_RENDERER_H__

#include "my_device.h"
#include "my_swap_chain.h"
#include "my_window.h"

// std
#include <cassert>
#include <memory>
#include <vector>

class MyRenderer 
{
public:
    MyRenderer(MyWindow& window, MyDevice& device);
    ~MyRenderer();

    MyRenderer(const MyRenderer&) = delete;
    MyRenderer& operator=(const MyRenderer&) = delete;
	MyRenderer(MyRenderer&&) = delete;
    MyRenderer& operator=(const MyRenderer&&) = delete;

    VkRenderPass    swapChainRenderPass() const { return m_mySwapChain->renderPass(); }
    VkRenderPass    pickRenderPass()      const { return m_mySwapChain->pickRenderPass(); }
    float           aspectRatio()         const { return m_mySwapChain->extentAspectRatio(); }
    size_t          imageCount()          const { return m_mySwapChain->imageCount(); }

    VkCommandBuffer beginFrame();
    void            endFrame();
    void            beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void            endSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void            recreateSwapChain();

    int frameIndex() const 
	{
        assert(m_bIsFrameStarted && "Cannot get frame index when frame not in progress");
        return m_iCurrentFrameIndex;
    }

    // Pick
	VkExtent2D      swapChainExtent();
    void            beginPickRenderPass(VkCommandBuffer commandBuffer, int x, int y, bool bDebug);
    void            endPickRenderPass(VkCommandBuffer commandBuffer);
	
private:
    void _createCommandBuffers();
    void _freeCommandBuffers();
    void _recreateSwapChain();

    VkCommandBuffer _currentCommandBuffer() const
    {
        assert(m_bIsFrameStarted && "Cannot get command buffer when frame not in progress");
        return m_vVkCommandBuffers[m_iCurrentFrameIndex];
    }

    MyWindow&                    m_myWindow;
    MyDevice&                    m_myDevice;
    std::unique_ptr<MySwapChain> m_mySwapChain;
    std::vector<VkCommandBuffer> m_vVkCommandBuffers;

    uint32_t                     m_iCurrentImageIndex;
    int                          m_iCurrentFrameIndex;
    bool                         m_bIsFrameStarted;
};

#endif

