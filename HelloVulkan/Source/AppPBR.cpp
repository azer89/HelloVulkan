#include "AppPBR.h"
#include "Configs.h"
#include "RendererEquirect2Cube.h"
#include "RendererCubeFilter.h"
#include "RendererBRDFLUT.h"
#include "RendererAABB.h"

#include "glm/gtc/matrix_transform.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_volk.h"

#include <random>

template <typename T>
inline T RandomNumber()
{
	static std::uniform_real_distribution<T> distribution(0.0, 1.0);
	static std::random_device rd;
	static std::mt19937 generator(rd());
	return distribution(generator);
}

// Returns a random real in [min,max)
template <typename T>
inline T RandomNumber(T min, T max)
{
	return min + (max - min) * RandomNumber<T>();
}

AppPBR::AppPBR() :
	modelRotation_(0.f)
{
}

void AppPBR::Init()
{
	// Initialize attachments
	CreateSharedImageResources();

	// Initialize lights
	InitLights();

	// Clustered forward buffers
	cfBuffers_.CreateBuffers(vulkanDevice_, lights_.GetLightCount());

	std::string hdrFile = AppConfig::TextureFolder + "dikhololo_night_4k.hdr";

	model_ = std::make_unique<Model>(
		vulkanDevice_, 
		AppConfig::ModelFolder + "Sponza//Sponza.gltf");
	std::vector<Model*> models = {model_.get()};

	// Create a cubemap from the input HDR
	{
		RendererEquirect2Cube e2c(
			vulkanDevice_,
			hdrFile);
		e2c.OffscreenRender(vulkanDevice_,
			&environmentCubemap_); // Output
		environmentCubemap_.SetDebugName(vulkanDevice_, "Environment_Cubemap");
	}

	// Cube filtering
	{
		RendererCubeFilter cubeFilter(vulkanDevice_, &environmentCubemap_);
		// Diffuse
		cubeFilter.OffscreenRender(vulkanDevice_,
			&diffuseCubemap_,
			CubeFilterType::Diffuse);
		// Specular
		cubeFilter.OffscreenRender(vulkanDevice_,
			&specularCubemap_,
			CubeFilterType::Specular);

		diffuseCubemap_.SetDebugName(vulkanDevice_, "Diffuse_Cubemap");
		specularCubemap_.SetDebugName(vulkanDevice_, "Specular_Cubemap");

		cubemapMipmapCount_ = NumMipMap(IBLConfig::InputCubeSideLength, IBLConfig::InputCubeSideLength);
	}
	
	// BRDF look up table
	{
		RendererBRDFLUT brdfLUTCompute(vulkanDevice_);
		brdfLUTCompute.CreateLUT(vulkanDevice_, &brdfLut_);
		brdfLut_.SetDebugName(vulkanDevice_, "BRDF_LUT");
	}

	{
		RendererAABB aabbCompute(vulkanDevice_);
		ClusterForwardUBO ubo = camera_->GetClusterForwardUBO();
		aabbCompute.CreateClusters(vulkanDevice_, ubo, &(cfBuffers_.aabbBuffer_));
	}

	// Renderers
	// This is responsible to clear swapchain image
	clearPtr_ = std::make_unique<RendererClear>(
		vulkanDevice_);
	// This draws a cube
	skyboxPtr_ = std::make_unique<RendererSkybox>(
		vulkanDevice_,
		&environmentCubemap_,
		&depthImage_,
		&multiSampledColorImage_,
		// This is the first offscreen render pass so
		// we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | 
		RenderPassBit::DepthClear
	);
	// This draws meshes with PBR+IBL
	pbrPtr_ = std::make_unique<RendererPBR>(
		vulkanDevice_,
		models,
		&lights_,
		&cfBuffers_,
		&specularCubemap_,
		&diffuseCubemap_,
		&brdfLut_,
		&depthImage_,
		&multiSampledColorImage_);
	lightPtr_ = std::make_unique<RendererLight>(
		vulkanDevice_,
		&lights_,
		&depthImage_,
		&multiSampledColorImage_
	);
	// aabbDebugPtr_
	aabbDebugPtr_ = std::make_unique<RendererAABBDebug>(
		vulkanDevice_,
		&cfBuffers_,
		&depthImage_,
		&multiSampledColorImage_
	);
	aabbDebugPtr_->SetInverseCameraUBO(vulkanDevice_, camera_.get());
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	resolveMSPtr_ = std::make_unique<RendererResolveMS>(
		vulkanDevice_, &multiSampledColorImage_, &singleSampledColorImage_);
	// This is on-screen render pass that transfers 
	// singleSampledColorImage_ to swapchain image
	tonemapPtr_ = std::make_unique<RendererTonemap>(
		vulkanDevice_,
		&singleSampledColorImage_
	);
	// ImGui here
	imguiPtr_ = std::make_unique<RendererImGui>(vulkanDevice_, vulkanInstance_.GetInstance(), glfwWindow_);
	// Present swapchain image
	finishPtr_ = std::make_unique<RendererFinish>(vulkanDevice_);

	// Put all renderer pointers to a vector
	renderers_ =
	{
		// Must be in order
		clearPtr_.get(),
		skyboxPtr_.get(),
		pbrPtr_.get(),
		lightPtr_.get(),
		//aabbDebugPtr_.get(),

		resolveMSPtr_.get(),
		tonemapPtr_.get(),
		imguiPtr_.get(),
		finishPtr_.get()
	};

	// Compute
	cullLightsPtr_ = std::make_unique<RendererCullLights>(vulkanDevice_, &lights_, &cfBuffers_);
}

void AppPBR::InitLights()
{
	std::vector<LightData> lights;

	float pi2 = glm::two_pi<float>();
	constexpr unsigned int NR_LIGHTS = 1000;
	for (unsigned int i = 0; i < NR_LIGHTS; ++i)
	{
		float yPos = RandomNumber<float>(-2.f, 20.0f);
		float radius = RandomNumber<float>(0.0f, 50.0f);
		float rad = RandomNumber<float>(0.0f, pi2);
		float xPos = glm::cos(rad);

		glm::vec4 position(
			glm::cos(rad) * radius,
			yPos,
			glm::sin(rad) * radius,
			1.f
		);

		glm::vec4 color(
			RandomNumber<float>(0.0f, 1.0f),
			RandomNumber<float>(0.0f, 1.0f),
			RandomNumber<float>(0.0f, 1.0f),
			1.f
		);

		LightData l;
		l.color_ = color;
		l.position_ = position;
		l.radius_ = RandomNumber<float>(2.0f, 6.0f);

		lights.push_back(l);
	}

	lights_.AddLights(vulkanDevice_, lights);
}

void AppPBR::DestroyResources()
{
	// Destroy images
	environmentCubemap_.Destroy(vulkanDevice_.GetDevice());
	diffuseCubemap_.Destroy(vulkanDevice_.GetDevice());
	specularCubemap_.Destroy(vulkanDevice_.GetDevice());
	brdfLut_.Destroy(vulkanDevice_.GetDevice());

	// Destroy meshes
	model_.reset();

	// Lights
	lights_.Destroy();

	// Clustered forward buffers
	cfBuffers_.Destroy(vulkanDevice_.GetDevice());

	// Destroy renderers
	clearPtr_.reset();
	finishPtr_.reset();
	skyboxPtr_.reset();
	pbrPtr_.reset();
	lightPtr_.reset();
	resolveMSPtr_.reset();
	tonemapPtr_.reset();
	imguiPtr_.reset();
	cullLightsPtr_.reset();
	aabbDebugPtr_.reset();
}

void AppPBR::UpdateUBOs(uint32_t imageIndex)
{
	// Per frame UBO
	// TODO Move to camera class
	PerFrameUBO skyboxUBO
	{
		.cameraProjection = camera_->GetProjectionMatrix(),
		.cameraView = glm::mat4(glm::mat3(camera_->GetViewMatrix())), // Remove translation
		.cameraPosition = glm::vec4(camera_->Position(), 1.f)
	};
	skyboxPtr_->SetPerFrameUBO(vulkanDevice_, imageIndex, skyboxUBO);
	// TODO Move to camera class
	PerFrameUBO pbrUBO
	{
		.cameraProjection = camera_->GetProjectionMatrix(),
		.cameraView = camera_->GetViewMatrix(),
		.cameraPosition = glm::vec4(camera_->Position(), 1.f)
	};
	aabbDebugPtr_->SetPerFrameUBO(vulkanDevice_, imageIndex, pbrUBO);
	lightPtr_->SetPerFrameUBO(vulkanDevice_, imageIndex, pbrUBO);
	pbrPtr_->SetPerFrameUBO(vulkanDevice_, imageIndex, pbrUBO);
	pbrPtr_->SeClusterForwardUBO(vulkanDevice_, imageIndex, camera_->GetClusterForwardUBO());

	// Model UBOs
	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::rotate(modelMatrix, modelRotation_, glm::vec3(0.f, 1.f, 0.f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(4.f));
	//modelRotation_ += deltaTime_ * 0.1f;

	// 1
	ModelUBO modelUBO1
	{
		.model = modelMatrix
	};
	model_->SetModelUBO(vulkanDevice_, imageIndex, modelUBO1);

	cullLightsPtr_->ResetGlobalIndex(vulkanDevice_, imageIndex);
	cullLightsPtr_->SetClusterForwardUBO(vulkanDevice_, imageIndex, camera_->GetClusterForwardUBO());

	camera_->ProcessMouseMovement(deltaTime_ * 20.0, 0.0);

}

void AppPBR::UpdateUI()
{
	if (!showImgui_)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::End();
		ImGui::Render();

		return;
	}

	static bool lightRender = true;
	static float lightIntensity = 1.f;
	static float pbrBaseReflectivity = 0.04f; // F0
	static float maxReflectivityLod = 4.0f;
	static float attenuationF = 1.0f;
	static float ambientStrength = 0.01f;

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowSize(ImVec2(525, 280));
	ImGui::Begin(AppConfig::ScreenTitle.c_str());

	ImGui::SetWindowFontScale(1.25f);
	ImGui::Text("FPS : %.0f", (1.f / deltaTime_));
	ImGui::Checkbox("Render Lights", &lightRender);
	ImGui::SliderFloat("Light Intensity", &lightIntensity, 0.1f, 100.f);
	ImGui::SliderFloat("Light Falloff", &attenuationF, 1.f, 20.f);
	ImGui::SliderFloat("Ambient Strength", &ambientStrength, 0.0f, 1.0f);
	ImGui::SliderFloat("Base Reflectivity", &pbrBaseReflectivity, 0.01f, 1.f);
	ImGui::SliderFloat("Max Mipmap Lod", &maxReflectivityLod, 0.1f, cubemapMipmapCount_);
	

	if (ImGui::Button("Reset Frustum"))
	{
		aabbDebugPtr_->SetInverseCameraUBO(vulkanDevice_, camera_.get());
	}

	ImGui::End();
	ImGui::Render();

	lightPtr_->RenderEnable(lightRender);
	pbrPtr_->SetLightIntensity(lightIntensity);
	pbrPtr_->SetBaseReflectivity(pbrBaseReflectivity);
	pbrPtr_->SetMaxReflectionLod(maxReflectivityLod);
	pbrPtr_->SetAtenuationF(attenuationF);
	pbrPtr_->SetAmbientStrength(ambientStrength);
}

void AppPBR::FillComputeCommandBuffer(VkCommandBuffer compCommandBuffer, uint32_t imageIndex)
{
	cullLightsPtr_->FillComputeCommandBuffer(vulkanDevice_, compCommandBuffer, imageIndex);
}

// This is called from main.cpp
int AppPBR::MainLoop()
{
	Init();

	// Main loop
	while (!GLFWWindowShouldClose())
	{
		PollEvents();
		ProcessTiming();
		ProcessInput();
		DrawFrame();
	}

	// Wait until everything is finished
	vkDeviceWaitIdle(vulkanDevice_.GetDevice());

	DestroyResources();

	Terminate();

	return 0;
}