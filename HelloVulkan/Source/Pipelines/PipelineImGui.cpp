#include "PipelineImGui.h"
#include "VulkanUtility.h"
#include "FrameCounter.h"
#include "PushConstants.h"
#include "Configs.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

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

	constexpr uint32_t imageCount = AppConfig::FrameCount;
	VulkanDescriptorInfo dsInfo;
	dsInfo.AddImage(nullptr);
	// TODO Layout is unused here
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, imageCount, 1u, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
	
	// ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();

	// Known issue when using both ImGui and volk
	// github.com/ocornut/imgui/issues/4854
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

void PipelineImGui::ImGuiStart()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void PipelineImGui::ImGuiSetWindow(const char* title, int width, int height, float fontSize)
{
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)));
	ImGui::Begin(title);
	ImGui::SetWindowFontScale(fontSize);
}

void PipelineImGui::ImGuiShowFrameData(FrameCounter* frameCounter)
{
	ImGui::Text("FPS: %.0f", frameCounter->GetCurrentFPS());
	ImGui::Text("Delta: %.0f ms", frameCounter->GetDelayedDeltaMillisecond());
	ImGui::PlotLines("FPS",
		frameCounter->GetGraph(),
		frameCounter->GetGraphLength(),
		0,
		nullptr,
		FLT_MAX,
		FLT_MAX,
		ImVec2(450, 50));
}

void PipelineImGui::ImGuiShowPBRConfig(PushConstPBR* pc, float mipmapCount)
{
	ImGui::SliderFloat("Light Falloff", &(pc->lightFalloff), 0.01f, 5.f);
	ImGui::SliderFloat("Light Intensity", &(pc->lightIntensity), 0.1f, 20.f);
	ImGui::SliderFloat("Albedo Multiplier", &(pc->albedoMultipler), 0.0f, 1.0f);
	ImGui::SliderFloat("Base Reflectivity", &(pc->baseReflectivity), 0.01f, 1.f);
	ImGui::SliderFloat("Max Mipmap Lod", &(pc->maxReflectionLod), 0.1f, mipmapCount);
}

void PipelineImGui::ImGuiEnd()
{
	ImGui::End();
	ImGui::Render();
}

void PipelineImGui::ImGuiDrawEmpty()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Render();
}

void PipelineImGui::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "ImGui", tracy::Color::DarkSeaGreen);
	const uint32_t swapchainImageIndex = ctx.GetCurrentSwapchainImageIndex();
	ImDrawData* draw_data = ImGui::GetDrawData();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
	vkCmdEndRenderPass(commandBuffer);
}