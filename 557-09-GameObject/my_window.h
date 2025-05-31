#ifndef __MY_WINDOW_H__
#define __MY_WINDOW_H__

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// std
#include <string>

class MyWindow
{
public:
	MyWindow(int w, int h, std::string name);
	~MyWindow();

	// Cannot do copy contrucror or assignment
	// because we have a GLGWwindow pointer
	MyWindow(const MyWindow&) = delete;
	MyWindow& operator=(const MyWindow&) = delete;
	MyWindow(MyWindow&&) = delete;
	MyWindow& operator=(const MyWindow&&) = delete;

	bool       shouldClose()            { return glfwWindowShouldClose(m_pWindow); }
	VkExtent2D extent()                 { return { static_cast<uint32_t>(m_iWidth), static_cast<uint32_t>(m_iHeight) }; };
	bool       wasWindowResized()       { return m_bframeBufferResize; }
	void       resetWindowResizedFlag() { m_bframeBufferResize = false; }

	void       createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);
	void       pollEvents();

private:
	static void s_frameBufferResizeCallback(GLFWwindow* window, int width, int height);
	void _initWindow();

	int            m_iWidth;
	int            m_iHeight;
	bool           m_bframeBufferResize = false;

	std::string    m_sWindowName;
	GLFWwindow*    m_pWindow;
};

#endif

