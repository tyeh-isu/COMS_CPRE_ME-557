#include "my_gui.h"
#include "imgui/imgui_internal.h"
#include <chrono>
#include <stdexcept>

static void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

/*MyGUI::MyGUI() :
	m_myDevice{ nullptr },
	m_myWindow{nullptr},
	m_myRenderer{ nullptr }
{
}

MyGUI::init(std::string title, MyDevice& device, MyWindow& window, MyRenderer& renderer)
{
	m_myDevice = device;
	m_myWindow = window;
	m_myRenderer = renderer;
	m_minImageCount = 2;
	m_sTitle = title;

	_init();
}*/

MyGUI::MyGUI(std::string title,MyDevice& device, MyWindow& window, MyRenderer& renderer)
	: m_myDevice{ device },
	  m_myWindow{ window },
	  m_myRenderer{ renderer },
	  m_minImageCount{ 2 },
	  m_sTitle(title)
{
	_init();
}

MyGUI::~MyGUI()
{
	// Cleanup
	vkDeviceWaitIdle(m_myDevice.device());
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	if (ImGui::GetCurrentContext()) 
	{
		ImGui::DestroyContext();
	}

	vkDestroyDescriptorPool(m_myDevice.device(), m_myDescriptorPool, nullptr);
}

void MyGUI::_init()
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

	// Create Descriptor Pool
	// The example only requires a single combined image sampler descriptor for the font image and only uses one descriptor set (for that)
	// If you wish to load e.g. additional textures you may need to alter pools sizes.
	{
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1;
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		if (vkCreateDescriptorPool(m_myDevice.device(), &pool_info, nullptr, &m_myDescriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("unable to create descriptor pool for imgui");
		}
	}

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForVulkan(m_myWindow.glfwWindow(), true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_myDevice.instance();
	init_info.PhysicalDevice = m_myDevice.physicalDevice();
	init_info.Device = m_myDevice.device();
	init_info.QueueFamily = m_myDevice.findPhysicalQueueFamilies().graphicsFamily;
	init_info.Queue = m_myDevice.graphicsQueue();
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = m_myDescriptorPool;
	init_info.RenderPass = m_myRenderer.swapChainRenderPass();
	init_info.Subpass = 0;
	init_info.MinImageCount = m_minImageCount;
	init_info.ImageCount = (uint32_t)m_myRenderer.imageCount();
	init_info.MSAASamples = m_myDevice.msaaSamples();
	init_info.Allocator = nullptr; // Don't use allocator for now
	init_info.CheckVkResultFn = check_vk_result;

	if (!ImGui_ImplVulkan_Init(&init_info))
	{
		throw std::runtime_error("Failed to initialize imgui for Vulkan");
	}
}

void MyGUI::draw(VkCommandBuffer commandBuffer, MyGUIData& data)
{
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//ImGuiWindowFlags window_flags = 0;
	//window_flags |= ImGuiWindowFlags_AlwaysAutoResize;

	// Main body of the Demo window starts here.
	ImGui::Begin(m_sTitle.c_str());

	// Design your GUI here
	_guiLayout(data);

	// Rendering
	ImGui::End();

	//ImGui::PopStyleVar();
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

	// Record dear imgui primitives into command buffer
	ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
}

void MyGUI::_guiLayout(MyGUIData& data)
{
	ImGui::Text("Light Position and Color");
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Checkbox("Show Light", &data.bShowLight);
	ImGui::SliderFloat("Move X", &data.fMoveValues[0], 0.0f, 1.0f);
	ImGui::SliderFloat("Move Y", &data.fMoveValues[1], 0.0f, 1.0f);
	ImGui::SliderFloat("Move Z", &data.fMoveValues[2], 0.0f, 1.0f);

	//ImGui::SliderFloat("Float Value", &data.fMoveValue, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
	ImGui::ColorEdit3("Color Values", (float*)&data.vColor);
}
