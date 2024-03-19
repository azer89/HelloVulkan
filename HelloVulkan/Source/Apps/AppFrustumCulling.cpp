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
	InitSharedResources();

	// Image-Based Lighting
	resIBL_ = std::make_unique<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");
	cubemapMipmapCount_ = static_cast<float>(Utility::MipMapCount(IBLConfig::InputCubeSideLength));

	InitScene();

	// Pipelines
	// This is responsible to clear swapchain image
	clearPtr_ = std::make_unique<PipelineClear>(vulkanContext_);
	// This draws a cube
	skyboxPtr_ = std::make_unique<PipelineSkybox>(
		vulkanContext_,
		&(resIBL_->diffuseCubemap_),
		resShared_.get(),
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	cullingPtr_ = std::make_unique<PipelineFrustumCulling>(vulkanContext_, scene_.get());
	pbrPtr_ = std::make_unique<PipelinePBRBindless>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_.get(),
		resIBL_.get(),
		resShared_.get());
	lightPtr_ = std::make_unique<PipelineLightRender>(
		vulkanContext_,
		resourcesLight_.get(),
		resShared_.get());
	linePtr_ = std::make_unique<PipelineLine>(vulkanContext_, resShared_.get(), scene_.get());
	boxRenderPtr_ = std::make_unique<PipelineAABBRender>(vulkanContext_, resShared_.get(), scene_.get());
	infGridPtr_ = std::make_unique<PipelineInfiniteGrid>(vulkanContext_, resShared_.get(), 0.0f);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	resolveMSPtr_ = std::make_unique<PipelineResolveMS>(vulkanContext_, resShared_.get());
	// This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	tonemapPtr_ = std::make_unique<PipelineTonemap>(vulkanContext_, &(resShared_->singleSampledColorImage_));
	// ImGui here
	imguiPtr_ = std::make_unique<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	// Present swapchain image
	finishPtr_ = std::make_unique<PipelineFinish>(vulkanContext_);

	// Put all renderer pointers to a vector
	pipelines_ =
	{
		// Must be in order
		clearPtr_.get(),
		skyboxPtr_.get(),
		cullingPtr_.get(),
		pbrPtr_.get(),
		infGridPtr_.get(),
		boxRenderPtr_.get(),
		linePtr_.get(),
		lightPtr_.get(),
		resolveMSPtr_.get(),
		tonemapPtr_.get(),
		imguiPtr_.get(),
		finishPtr_.get()
	};
}

void AppFrustumCulling::InitScene()
{
	constexpr uint32_t xCount = 50;
	constexpr uint32_t zCount = 50;
	constexpr float dist = 4.0f;
	constexpr float xMidPos = static_cast<float>(xCount - 1) * dist / 2.0f;
	constexpr float zMidPos = static_cast<float>(zCount - 1) * dist / 2.0f;
	std::vector<ModelData> dataArray = { { AppConfig::ModelFolder + "Zaku/Zaku.gltf", xCount * zCount} };
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
	resourcesLight_ = std::make_unique<ResourcesLight>();
	resourcesLight_->AddLights(vulkanContext_,
		{
			{.position_ = glm::vec4(-1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
			{.position_ = glm::vec4(1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
			{.position_ = glm::vec4(-1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
			{.position_ = glm::vec4(1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
		});
}

void AppFrustumCulling::DestroyResources()
{
	// Resources
	resIBL_.reset();
	resourcesLight_.reset();

	// Destroy meshes
	scene_.reset();

	// Destroy renderers
	clearPtr_.reset();
	finishPtr_.reset();
	skyboxPtr_.reset();
	cullingPtr_.reset();
	pbrPtr_.reset();
	lightPtr_.reset();
	boxRenderPtr_.reset();
	resolveMSPtr_.reset();
	tonemapPtr_.reset();
	imguiPtr_.reset();
	linePtr_.reset();
	infGridPtr_.reset();
}

void AppFrustumCulling::UpdateUBOs()
{
	CameraUBO ubo = camera_->GetCameraUBO();
	lightPtr_->SetCameraUBO(vulkanContext_, ubo);
	pbrPtr_->SetCameraUBO(vulkanContext_, ubo);
	boxRenderPtr_->SetCameraUBO(vulkanContext_, ubo);
	infGridPtr_->SetCameraUBO(vulkanContext_, ubo);
	linePtr_->SetCameraUBO(vulkanContext_, ubo);

	if (updateFrustum_)
	{
		linePtr_->SetFrustum(vulkanContext_, ubo);

		FrustumUBO frustumUBO = camera_->GetFrustumUBO();
		cullingPtr_->SetFrustumUBO(vulkanContext_, frustumUBO);
	}

	// Remove translation
	CameraUBO skyboxUbo = ubo;
	skyboxUbo.view = glm::mat4(glm::mat3(skyboxUbo.view));
	skyboxPtr_->SetCameraUBO(vulkanContext_, skyboxUbo);
}

void AppFrustumCulling::UpdateUI()
{
	if (!showImgui_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	static bool staticLightRender = false;
	static bool staticLineRender = false;
	static bool staticUpdateFrustum = true;
	static PushConstPBR staticPBRPushConstants =
	{
		.albedoMultipler = 0.5f
	};

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Compute-Based Frustum Culling", 525, 350);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	ImGui::Checkbox("Render Lights", &staticLightRender);
	ImGui::Checkbox("Render Frustum and Bounding Boxes", &staticLineRender);
	ImGui::Checkbox("Update Frustum", &staticUpdateFrustum);
	imguiPtr_->ImGuiShowPBRConfig(&staticPBRPushConstants, cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

	updateFrustum_ = staticUpdateFrustum;

	lightPtr_->ShouldRender(staticLightRender);
	linePtr_->ShouldRender(staticLineRender);
	boxRenderPtr_->ShouldRender(staticLineRender);
	pbrPtr_->SetPBRPushConstants(staticPBRPushConstants);
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

	DestroyResources();
	DestroyInternal();
}