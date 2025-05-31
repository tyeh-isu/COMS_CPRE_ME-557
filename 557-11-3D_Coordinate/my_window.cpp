#include "my_window.h"
#include <stdexcept>

MyWindow::MyWindow(int w, int h, std::string name) : 
	m_iWidth(w),
	m_iHeight(h),
	m_sWindowName(name),
	m_pWindow(nullptr),
	m_bframeBufferResize(false)
{
	_initWindow();
}

MyWindow::~MyWindow()
{
	glfwDestroyWindow(m_pWindow);
	glfwTerminate();
}

void MyWindow::_initWindow()
{
	glfwInit();

	// GLFW was designed initially to create OpenGL context by default.
	// By setting GLFW_CLIENT_API to GLFW_NO_API, it tells GLFW NOT to create OpenGL context 
	// (because we are going to use Vulkan)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Need to handle the window resizing in a specical way later in Vulkan code
	// so we need to set it to false initally until frameBufferResizeCallback is set
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_pWindow = glfwCreateWindow(m_iWidth, m_iHeight, m_sWindowName.c_str(), nullptr, nullptr);

	// For the call back function to use this pointer
	glfwSetWindowUserPointer(m_pWindow, this);

	// Register viewport resize callback
	glfwSetFramebufferSizeCallback(m_pWindow, s_frameBufferResizeCallback);
}

void MyWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	if (glfwCreateWindowSurface(instance, m_pWindow, nullptr, surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window");
	}
}

void MyWindow::s_frameBufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto mywindow = reinterpret_cast<MyWindow*>(glfwGetWindowUserPointer(window));
	mywindow->m_bframeBufferResize = true;
	mywindow->m_iWidth = width;
	mywindow->m_iHeight = height;
}

void MyWindow::pollEvents()
{
	glfwPollEvents();
}

