#include "my_window.h"
#include "my_application.h"
#include <stdexcept>

MyWindow::MyWindow(int w, int h, std::string name) : 
	m_iWidth(w),
	m_iHeight(h),
	m_sWindowName(name),
	m_pWindow(nullptr),
	m_bframeBufferResize(false),
	m_pMyApplication(nullptr)
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

	// Register keyboard callback	
	glfwSetKeyCallback(m_pWindow, s_keyboardCallback);
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

void MyWindow::s_keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Handle keyboard events
	auto mywindow = reinterpret_cast<MyWindow*>(glfwGetWindowUserPointer(window));
	
	if ((key == GLFW_KEY_C || key == GLFW_KEY_ESCAPE) && 
		 action == GLFW_PRESS)
	{
		mywindow->keyboardEvent(key);
	}
}

void MyWindow::pollEvents()
{
	glfwPollEvents();

	bool bUpdateLight = false;
	glm::vec3 lightUpdate{0.0f};

	if (glfwGetKey(m_pWindow, GLFW_KEY_K) == GLFW_PRESS)
	{ 
		bUpdateLight = true;
		lightUpdate.x -= 0.0001f;
	}
    else if (glfwGetKey(m_pWindow, GLFW_KEY_L) == GLFW_PRESS)
	{
		bUpdateLight = true;
		lightUpdate.x += 0.0001f;
	}

	if (m_pMyApplication && bUpdateLight)
	{
		m_pMyApplication->updateLightPosition(lightUpdate);
	}
}

void MyWindow::waitEvents()
{
	glfwWaitEvents();
}

const char** MyWindow::getRequiredInstanceExtensions(uint32_t* extensionCount)
{
	return glfwGetRequiredInstanceExtensions(extensionCount);
}

void MyWindow::bindMyApplication(MyApplication* pMyApplication)
{
	m_pMyApplication = pMyApplication;
}

void MyWindow::keyboardEvent(int key)
{
	if (m_pMyApplication == nullptr) return;

	if (key == GLFW_KEY_C)
	{
		m_pMyApplication->switchProjectionMatrix();
	}
	else if (key == GLFW_KEY_ESCAPE)
	{
		glfwSetWindowShouldClose(m_pWindow, 1);
	}
}

