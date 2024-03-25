#include "AppFrustumCulling.h"
#include "VulkanUtility.h"
#include "Configs.h"

#include "glm/ext.hpp"
#include "imgui_impl_vulkan.h"

AppFrustumCulling::AppFrustumCulling() :
	updateFrustum_(true)
{
}

void AppFrustumCulling::Init()
{
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 10.0f, 5.0f), glm::vec3(0.0, 0.0, -20));

	InitLights();

	// Initialize attachments
	InitSharedResources2();

	// Image-Based Lighting
	resIBL2_ = AddResources<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");

	InitScene();

	// Pipelines
	AddPipeline<PipelineClear>(vulkanContext_); // This is responsible to clear swapchain image
	AddPipeline<PipelineSkybox>(
		vulkanContext_,
		&(resIBL2_->diffuseCubemap_),
		resShared2_,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	cullingPtr_ = AddPipeline<PipelineFrustumCulling>(vulkanContext_, scene_.get());
	pbrPtr_ = AddPipeline<PipelinePBRBindless>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_,
		resIBL2_,
		resShared2_);
	infGridPtr_ = AddPipeline<PipelineInfiniteGrid>(vulkanContext_, resShared2_, 0.0f);
	boxRenderPtr_ = AddPipeline<PipelineAABBRender>(vulkanContext_, resShared2_, scene_.get());
	linePtr_ = AddPipeline<PipelineLine>(vulkanContext_, resShared2_, scene_.get());
	lightPtr_ = AddPipeline<PipelineLightRender>(
		vulkanContext_,
		resourcesLight_,
		resShared2_);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	AddPipeline<PipelineResolveMS>(vulkanContext_, resShared2_);
	// This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	AddPipeline<PipelineTonemap>(vulkanContext_, &(resShared2_->singleSampledColorImage_));
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	// Present swapchain image
	AddPipeline<PipelineFinish>(vulkanContext_);

	// ImGui
	inputContext_.pbrPC_.albedoMultipler = 0.5f;
}

void AppFrustumCulling::InitScene()
{
	constexpr uint32_t xCount = 50;
	constexpr uint32_t zCount = 50;
	constexpr float dist = 4.0f;
	constexpr float xMidPos = static_cast<float>(xCount - 1) * dist / 2.0f;
	constexpr float zMidPos = static_cast<float>(zCount - 1) * dist / 2.0f;
	std::vector<ModelCreateInfo> dataArray = { { AppConfig::ModelFolder + "Zaku/Zaku.gltf", xCount * zCount} };
	bool supportDeviceAddress = true;
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray, supportDeviceAddress);
	uint32_t iter = 0;

	for (int x = 0; x < xCount; ++x)
	{
		for (int z = 0; z < zCount; ++z)
		{
			float xPos = x * dist - xMidPos;
			float yPos = 0.0f;
			float zPos = -(z * dist) + zMidPos;
			glm::mat4 modelMatrix(1.f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(xPos, yPos, zPos));
			scene_->UpdateModelMatrix(vulkanContext_, { .model = modelMatrix }, 0, iter++);
		}
	}
}

void AppFrustumCulling::InitLights()
{
	// Lights (SSBO)
	resourcesLight_ = AddResources<ResourcesLight>();
	resourcesLight_->AddLights(vulkanContext_,
		{
			{.position_ = glm::vec4(-1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
			{.position_ = glm::vec4(1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
			{.position_ = glm::vec4(-1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
			{.position_ = glm::vec4(1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
		});
}

void AppFrustumCulling::UpdateUBOs()
{
	CameraUBO ubo = camera_->GetCameraUBO();
	for (auto& pipeline : pipelines2_)
	{
		pipeline->SetCameraUBO(vulkanContext_, ubo);
	}

	if (updateFrustum_)
	{
		linePtr_->SetFrustum(vulkanContext_, ubo);

		FrustumUBO frustumUBO = camera_->GetFrustumUBO();
		cullingPtr_->SetFrustumUBO(vulkanContext_, frustumUBO);
	}
}

void AppFrustumCulling::UpdateUI()
{
	if (!showImgui_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	static bool staticUpdateFrustum = true;

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Compute-Based Frustum Culling", 500, 350);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	ImGui::Checkbox("Render Lights", &inputContext_.renderLights_);
	ImGui::Checkbox("Render Frustum and Bounding Boxes", &inputContext_.renderDebug_);
	ImGui::Checkbox("Update Frustum", &staticUpdateFrustum);
	imguiPtr_->ImGuiShowPBRConfig(&inputContext_.pbrPC_, resIBL2_->cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

	updateFrustum_ = staticUpdateFrustum;

	lightPtr_->ShouldRender(inputContext_.renderLights_);
	linePtr_->ShouldRender(inputContext_.renderDebug_);
	boxRenderPtr_->ShouldRender(inputContext_.renderDebug_);
	pbrPtr_->SetPBRPushConstants(inputContext_.pbrPC_);
}

// This is called from main.cpp
void AppFrustumCulling::MainLoop()
{
	InitVulkan({
		.suportBufferDeviceAddress_ = true,
		.supportMSAA_ = true,
		.supportBindlessTextures_ = true,
		.supportWideLines_ = true
		});
	Init();

	// Main loop
	while (StillRunning())
	{
		PollEvents();
		ProcessTiming();
		ProcessInput();
		DrawFrame();
	}

	//DestroyResources();
	scene_.reset();
	
	DestroyInternal();
}