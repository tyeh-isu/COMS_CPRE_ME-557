#define GLFW_INCLUDE_VULKAN // This macro will include vulkan_core.h
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <vector>

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(800, 600, "Hello Vulkan!", nullptr, nullptr);

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::cout << extensionCount << " extensions supported\n";

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    
	std::cout << "Check Vulkan:" << std::endl;
	for (const auto &extension : extensions) 
	{
		std::cout << "\t" << extension.extensionName << std::endl;
	}

	std::cout << "Check GLM:" << std::endl;
	glm::mat4 matrix = glm::mat4(1.0f);
	glm::vec4 vec(1.0f, 0.0f, 0.0f, 1.0f);
	auto result = matrix * vec;
	std::cout << "vector result = " << result.x << " " << result.y << " " << result.z << " " << result.w << std::endl;

	std::cout << "Check GLFW:" << std::endl;
	std::cout << "display a \"Hello Vulkan!\" window" << std::endl;
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}

