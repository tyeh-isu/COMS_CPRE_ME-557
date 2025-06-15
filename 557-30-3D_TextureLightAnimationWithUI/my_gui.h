#ifndef __MY_GUI_H__
#define __MY_GUI_H__

#include "my_device.h"
#include "my_renderer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

struct MyGUIData
{
	// Add you state data here
	int    iMoveDirection;
	bool   bShowLight;
	float  fMoveValues[3];
	ImVec4 vColor;
	
	void init()
	{
		iMoveDirection = 0;
		bShowLight = true;
		fMoveValues[0] = 0.5f;
		fMoveValues[1] = 0.5f;
		fMoveValues[2] = 0.5f;
		vColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	}
};

class MyGUI
{
public:
	MyGUI(std::string title, MyDevice& device, MyWindow& window, MyRenderer& renderer);
	~MyGUI();

	void draw(VkCommandBuffer commandBuffer, MyGUIData &data);

	MyGUI(const MyGUI&) = delete;
	MyGUI& operator=(const MyGUI&) = delete;
	MyGUI(MyGUI&&) = delete;
	MyGUI& operator=(const MyGUI&&) = delete;

private:
	void _init();
	void _guiLayout(MyGUIData& data);

	MyDevice& m_myDevice;
	MyWindow& m_myWindow;
	MyRenderer& m_myRenderer;
	VkDescriptorPool m_myDescriptorPool;

	std::string m_sTitle;
	uint32_t    m_minImageCount;
};

#endif
