#include "RendererImGui.h"
#include "VulkanUtility.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"

// Known issue when using both ImGui and volk
// https://github.com/ocornut/imgui/issues/4854
#include "imgui_impl_volk.h"

RendererImGui::RendererImGui(
	VulkanDevice& vkDev,
	VkInstance vulkanInstance,
	GLFWwindow* glfwWindow) :
	RendererBase(vkDev, false) // Onscreen
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(vkDev);
	framebuffer_.Create(vkDev, renderPass_.GetHandle(), {}, isOffscreen_);

	CreateDescriptorPool(
		vkDev,
		0, // uniform
		0, // SSBO
		1, // Texture
		1, // One set per swapchain
		&descriptorPool_);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Known issue when using both ImGui and volk
	// https://github.com/ocornut/imgui/issues/4854
	ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* vulkan_instance)
	{
		return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(vulkan_instance)), function_name);
	}, &vulkanInstance);

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(glfwWindow, false);

	uint32_t imageCount = static_cast<uint32_t>(vkDev.GetSwapchainImageCount());

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = vulkanInstance;
	init_info.PhysicalDevice = vkDev.GetPhysicalDevice();
	init_info.Device = vkDev.GetDevice();
	init_info.QueueFamily = vkDev.GetGraphicsFamily();
	init_info.Queue = vkDev.GetGraphicsQueue();
	init_info.PipelineCache = nullptr;
	init_info.DescriptorPool = descriptorPool_;
	init_info.Subpass = 0;
	init_info.MinImageCount = imageCount;
	init_info.ImageCount = imageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.ColorAttachmentFormat = vkDev.GetSwaphchainImageFormat();
	init_info.Allocator = nullptr;
	
	ImGui_ImplVulkan_Init(&init_info, renderPass_.GetHandle());
}

RendererImGui::~RendererImGui()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void RendererImGui::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{

}

void RendererImGui::CreateDescriptorLayout(VulkanDevice& vkDev)
{
	/*uint32_t bindingIndex = 0u;

	std::vector<VkDescriptorSetLayoutBinding> bindings =
	{
		DescriptorSetLayoutBinding(
			bindingIndex++,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	const VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(
		vkDev.GetDevice(),
		&layoutInfo,
		nullptr,
		&descriptorSetLayout_));*/
}

void RendererImGui::AllocateDescriptorSets(VulkanDevice& vkDev)
{
	/*auto swapChainImageSize = vkDev.GetSwapchainImageCount();

	std::vector<VkDescriptorSetLayout> layouts(swapChainImageSize, descriptorSetLayout_);

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(swapChainImageSize),
		.pSetLayouts = layouts.data()
	};

	descriptorSets_.resize(swapChainImageSize);

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, descriptorSets_.data()));*/
}

void RendererImGui::UpdateDescriptorSets(VulkanDevice& vkDev)
{
}