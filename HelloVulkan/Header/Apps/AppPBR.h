#ifndef APP_PBR
#define APP_PBR

#include "AppBase.h"
#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelinePBR.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineLight.h"
#include "PipelineImGui.h"
#include "VulkanImage.h"
#include "Light.h"
#include "Model.h"

#include <vector>
#include <memory>

class AppPBR final : AppBase
{
public:
	AppPBR();
	int MainLoop() override;
	void UpdateUBOs(uint32_t imageIndex) override;
	void UpdateUI() override;

	void Init();
	void InitLights();
	void DestroyResources();

private:
	std::unique_ptr<PipelineClear> clearPtr_;
	std::unique_ptr<PipelineSkybox> skyboxPtr_;
	std::unique_ptr<PipelinePBR> pbrPtr_;
	std::unique_ptr<PipelineTonemap> tonemapPtr_;
	std::unique_ptr<PipelineFinish> finishPtr_;
	std::unique_ptr<PipelineResolveMS> resolveMSPtr_;
	std::unique_ptr<PipelineLight> lightPtr_;
	std::unique_ptr<PipelineImGui> imguiPtr_;

	// PBR stuff
	// TODO change to unique ptrs
	VulkanImage environmentCubemap_;
	VulkanImage diffuseCubemap_;
	VulkanImage specularCubemap_;
	VulkanImage brdfLut_;
	float cubemapMipmapCount_;

	float modelRotation_;
	std::unique_ptr<Model> model_;

	Lights lights_;
};

#endif