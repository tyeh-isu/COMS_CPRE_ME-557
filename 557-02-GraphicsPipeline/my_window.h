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

	// Cannot do copy or move contrucror or assignment
	// because we have a GLGWwindow pointer
	// C++ rule of five
	MyWindow(const MyWindow&) = delete;
	MyWindow& operator=(const MyWindow&) = delete;
	MyWindow(MyWindow &&) = delete;
	MyWindow& operator=(const MyWindow&&) = delete;

	bool shouldClose() { return glfwWindowShouldClose(m_pWindow); }

private:
	void _initWindow();

	const int   m_iWidth;
	const int   m_iHeight;

	std::string m_sWindowName;
	GLFWwindow* m_pWindow;
};

#endif

