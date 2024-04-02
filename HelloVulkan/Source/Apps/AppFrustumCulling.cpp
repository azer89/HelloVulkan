#include "AppFrustumCulling.h"
#include "Utility.h"
#include "Configs.h"

#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineInfiniteGrid.h"

#include "glm/ext.hpp"
#include "imgui_impl_vulkan.h"

AppFrustumCulling::AppFrustumCulling()
{
}

void AppFrustumCulling::Init()
{
	uiData_.pbrPC_.albedoMultipler = 0.5f;
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 10.0f, 5.0f), glm::vec3(0.0, 0.0, -20));

	InitLights();

	// Initialize attachments
	InitSharedResources();

	// Image-Based Lighting
	resourcesIBL_ = AddResources<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");

	InitScene();

	// Pipelines
	AddPipeline<PipelineClear>(vulkanContext_); // This is responsible to clear swapchain image
	AddPipeline<PipelineSkybox>(
		vulkanContext_,
		&(resourcesIBL_->diffuseCubemap_),
		resourcesShared_,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	cullingPtr_ = AddPipeline<PipelineFrustumCulling>(vulkanContext_, scene_.get());
	pbrPtr_ = AddPipeline<PipelinePBRBindless>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_,
		resourcesIBL_,
		resourcesShared_,
		false);
	infGridPtr_ = AddPipeline<PipelineInfiniteGrid>(vulkanContext_, resourcesShared_, 0.0f);
	boxRenderPtr_ = AddPipeline<PipelineAABBRender>(vulkanContext_, resourcesShared_, scene_.get());
	linePtr_ = AddPipeline<PipelineLine>(vulkanContext_, resourcesShared_, scene_.get());
	lightPtr_ = AddPipeline<PipelineLightRender>(
		vulkanContext_,
		resourcesLight_,
		resourcesShared_);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	AddPipeline<PipelineResolveMS>(vulkanContext_, resourcesShared_);
	// This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	AddPipeline<PipelineTonemap>(vulkanContext_, &(resourcesShared_->singleSampledColorImage_));
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_, scene_.get(), camera_.get());
	// Present swapchain image
	AddPipeline<PipelineFinish>(vulkanContext_);
}

void AppFrustumCulling::InitScene()
{
	constexpr uint32_t xCount = 50;
	constexpr uint32_t zCount = 50;
	constexpr float dist = 4.0f;
	constexpr float xMidPos = static_cast<float>(xCount - 1) * dist / 2.0f;
	constexpr float zMidPos = static_cast<float>(zCount - 1) * dist / 2.0f;
	std::vector<ModelCreateInfo> dataArray = {{ 
		.filename = AppConfig::ModelFolder + "Zaku/Zaku.gltf", 
		.instanceCount = xCount * zCount,
		.playAnimation = false
	}};
	bool supportDeviceAddress = true;
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray, supportDeviceAddress);
	uint32_t iter = 0;

	for (uint32_t x = 0; x < xCount; ++x)
	{
		for (uint32_t z = 0; z < zCount; ++z)
		{
			float xPos = static_cast<float>(x) * dist - xMidPos;
			float yPos = 0.0f;
			float zPos = -(static_cast<float>(z) * dist) + zMidPos;
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

void AppFrustumCulling::UpdateUI()
{
	if (!ShowImGui())
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	static bool staticUpdateFrustum = true;

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Compute-Based Frustum Culling", 450, 375);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	ImGui::Checkbox("Render Lights", &uiData_.renderLights_);
	ImGui::Checkbox("Render Frustum and Bounding Boxes", &uiData_.renderDebug_);
	ImGui::Checkbox("Update Frustum", &staticUpdateFrustum);
	imguiPtr_->ImGuiShowPBRConfig(&uiData_.pbrPC_, resourcesIBL_->cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

	updateFrustum_ = staticUpdateFrustum;

	for (auto& pipeline : pipelines_)
	{
		pipeline->UpdateFromIUData(vulkanContext_, uiData_);
	}
}

void AppFrustumCulling::UpdateUBOs()
{
	CameraUBO ubo = camera_->GetCameraUBO();
	for (auto& pipeline : pipelines_)
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
	
	DestroyResources();
}