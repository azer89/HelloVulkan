#include "PipelineImGui.h"
#include "VulkanUtility.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"

// Known issue when using both ImGui and volk
// https://github.com/ocornut/imgui/issues/4854
#include "imgui_impl_volk.h"

PipelineImGui::PipelineImGui(
	VulkanDevice& vkDev,
	VkInstance vulkanInstance,
	GLFWwindow* glfwWindow) :
	PipelineBase(vkDev, 
		{
			.type_ = PipelineType::GraphicsOnScreen
		}
	)
{
	// Create render pass
	renderPass_.CreateOnScreenColorOnlyRenderPass(vkDev);

	// Create framebuffer
	framebuffer_.Create(vkDev, renderPass_.GetHandle(), {}, IsOffscreen());

	uint32_t imageCount = static_cast<uint32_t>(vkDev.GetSwapchainImageCount());
	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = 0u,
			.ssboCount_ = 0u,
			.samplerCount_ = 1u,
			.swapchainCount_ = imageCount,
			.setCountPerSwapchain_ = 1u,
			.flags_ = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
		});
	
	// ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();

	// Known issue when using both ImGui and volk
	// https://github.com/ocornut/imgui/issues/4854
	ImGui_ImplVulkan_LoadFunctions([](const char* functionName, void* vulkanInstance)
	{
		return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(vulkanInstance)), functionName);
	}, &vulkanInstance);

	ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

	ImGui_ImplVulkan_InitInfo init_info = {
		.Instance = vulkanInstance,
		.PhysicalDevice = vkDev.GetPhysicalDevice(),
		.Device = vkDev.GetDevice(),
		.QueueFamily = vkDev.GetGraphicsFamily(),
		.Queue = vkDev.GetGraphicsQueue(),
		.PipelineCache = nullptr,
		.DescriptorPool = descriptor_.pool_,
		.Subpass = 0,
		.MinImageCount = imageCount,
		.ImageCount = imageCount,
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.ColorAttachmentFormat = vkDev.GetSwaphchainImageFormat(),
	};
	
	ImGui_ImplVulkan_Init(&init_info, renderPass_.GetHandle());
}

PipelineImGui::~PipelineImGui()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void PipelineImGui::StartImGui()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void PipelineImGui::EndImGui()
{
	ImGui::End();
	ImGui::Render();
}

void PipelineImGui::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer)
{
	uint32_t swapchainImageIndex = vkDev.GetCurrentSwapchainImageIndex();
	ImDrawData* draw_data = ImGui::GetDrawData();
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
	vkCmdEndRenderPass(commandBuffer);
}