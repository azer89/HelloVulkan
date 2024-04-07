#include "AppRaytracing.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"

#include "imgui.h"

AppRaytracing::AppRaytracing()
{
}

void AppRaytracing::Init()
{
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 3.0f, 3.0f), glm::vec3(0.0, 0.0, -1.0f));

	InitLights();

	// Scene
	std::vector<ModelCreateInfo> dataArray = {
		{
			.filename = AppConfig::ModelFolder + "MacrossFactory/MacrossFactory.gltf",
		}
	};
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray);

	AddPipeline<PipelineClear>(vulkanContext_);
	rtxPtr_ = AddPipeline<PipelineRaytracing>(vulkanContext_, scene_.get(), resourcesLight_);
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	AddPipeline<PipelineFinish>(vulkanContext_);

	// Add a listener when the camera is changed
	camera_->ChangedEvent_.AddListener([this]()
	{
		this->rtxPtr_->ResetFrameCounter();
	});
}

void AppRaytracing::InitLights()
{
	constexpr float lightIntensity = 15.f;
	constexpr float lightPositionY = 1.75f;

	// Lights (SSBO)
	resourcesLight_ = AddResources<ResourcesLight>();
	resourcesLight_->AddLights(vulkanContext_,
	{
		{.position_ = glm::vec4(-1.5f, lightPositionY,  1.5f, 1.f), .color_ = glm::vec4(lightIntensity), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, lightPositionY,  1.5f, 1.f), .color_ = glm::vec4(lightIntensity), .radius_ = 10.0f },
		{.position_ = glm::vec4(-1.5f, lightPositionY, -1.5f, 1.f), .color_ = glm::vec4(lightIntensity), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, lightPositionY, -1.5f, 1.f), .color_ = glm::vec4(lightIntensity), .radius_ = 10.0f }
	});
}

void AppRaytracing::UpdateUI()
{
	if (!ShowImGui())
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	static int sampleCountPerFrame = 4;
	static int rayBounceCount = 8;
	static float skyIntensity = 1.0f;

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Raytracing", 450, 150);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	ImGui::SliderInt("Sample count", &sampleCountPerFrame, 1, 32);
	ImGui::SliderInt("Ray bounce", &rayBounceCount, 1, 32);
	ImGui::SliderFloat("Sky intensity", &skyIntensity, 0.1f, 10.0f);
	imguiPtr_->ImGuiEnd();

	rtxPtr_->SetParams(
		static_cast<uint32_t>(sampleCountPerFrame),
		static_cast<uint32_t>(rayBounceCount),
		skyIntensity
	);
}

void AppRaytracing::UpdateUBOs()
{
	rtxPtr_->SetRaytracingUBO(vulkanContext_,
		camera_->GetInverseProjectionMatrix(),
		camera_->GetInverseViewMatrix(),
		camera_->Position()
	);
}

void AppRaytracing::MainLoop()
{
	InitVulkan({
		.supportRaytracing_ = true,
		.suportBufferDeviceAddress_ = true,
		.supportMSAA_ = true,
		.supportBindlessTextures_ = true,
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

	scene_.reset();
	DestroyResources();
}
