#include "PipelineImGui.h"
#include "VulkanUtility.h"
#include "Configs.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"

// Known issue when using both ImGui and volk
// https://github.com/ocornut/imgui/issues/4854
#include "imgui_impl_volk.h"

PipelineImGui::PipelineImGui(
	VulkanContext& ctx,
	VkInstance vulkanInstance,
	GLFWwindow* glfwWindow) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOnScreen
		}
	)
{
	// Create render pass
	renderPass_.CreateOnScreenColorOnlyRenderPass(ctx);

	// Create framebuffer
	framebuffer_.CreateResizeable(ctx, renderPass_.GetHandle(), {}, IsOffscreen());

	uint32_t imageCount = AppConfig::FrameOverlapCount;
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = 0u,
			.ssboCount_ = 0u,
			.samplerCount_ = 1u,
			.frameCount_ = imageCount,
			.setCountPerFrame_ = 1u,
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
		.PhysicalDevice = ctx.GetPhysicalDevice(),
		.Device = ctx.GetDevice(),
		.QueueFamily = ctx.GetGraphicsFamily(),
		.Queue = ctx.GetGraphicsQueue(),
		.PipelineCache = nullptr,
		.DescriptorPool = descriptor_.pool_,
		.Subpass = 0,
		.MinImageCount = imageCount,
		.ImageCount = imageCount,
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.ColorAttachmentFormat = ctx.GetSwapchainImageFormat(),
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

void PipelineImGui::DrawEmptyImGui()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Render();
}

void PipelineImGui::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	uint32_t swapchainImageIndex = ctx.GetCurrentSwapchainImageIndex();
	ImDrawData* draw_data = ImGui::GetDrawData();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
	vkCmdEndRenderPass(commandBuffer);
}