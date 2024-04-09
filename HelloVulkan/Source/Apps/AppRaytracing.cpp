#include "AppRaytracing.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"

#include "imgui.h"

AppRaytracing::AppRaytracing()
{
}

void AppRaytracing::Init()
{
	camera_->SetPositionAndTarget(glm::vec3(0.5f, 1.0f, 7.0f), glm::vec3(-3.0, 1.0, 3.5f));
	uiData_.shadowCasterPosition_ = { 1.0f, 1.0f, -0.3f };

	// Scene
	std::vector<ModelCreateInfo> dataArray = {
		{
			.filename = AppConfig::ModelFolder + "MacrossFactory/MacrossFactory.gltf",
		}
	};
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray);

	AddPipeline<PipelineClear>(vulkanContext_);
	rtxPtr_ = AddPipeline<PipelineRaytracing>(vulkanContext_, scene_.get());
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	AddPipeline<PipelineFinish>(vulkanContext_);

	// Add a listener when the camera is changed
	camera_->ChangedEvent_.AddListener([this]()
	{
		this->rtxPtr_->ResetFrameCounter();
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
	static int rayBounceCount = 4;
	static float skyIntensity = 1.0f;
	static float lightIntensity = 2.5f;
	static float specularFuzziness = 0.05f;

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Raytracing", 450, 150);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	ImGui::SliderInt("Sample count", &sampleCountPerFrame, 1, 32);
	ImGui::SliderInt("Ray bounce", &rayBounceCount, 1, 32);
	ImGui::SliderFloat("Sky intensity", &skyIntensity, 0.1f, 10.0f);
	ImGui::SliderFloat("Light intensity", &lightIntensity, 1.0f, 10.0f);
	ImGui::SliderFloat("Specular fuzzyness", &specularFuzziness, 0.01f, 1.0f);
	ImGui::SeparatorText("Shadow caster direction");
	ImGui::SliderFloat("X", &(uiData_.shadowCasterPosition_[0]), -1.0f, 1.0f);
	ImGui::SliderFloat("Y", &(uiData_.shadowCasterPosition_[1]),  0.0f, 1.0f);
	ImGui::SliderFloat("Z", &(uiData_.shadowCasterPosition_[2]), -1.0f, 1.0f);
	imguiPtr_->ImGuiEnd();

	rtxPtr_->SetParams(
		uiData_.shadowCasterPosition_,
		static_cast<uint32_t>(sampleCountPerFrame),
		static_cast<uint32_t>(rayBounceCount),
		skyIntensity,
		lightIntensity,
		specularFuzziness
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
