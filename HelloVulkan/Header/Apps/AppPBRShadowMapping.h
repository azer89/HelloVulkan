#ifndef APP_PBR_SHADOW_MAPPING
#define APP_PBR_SHADOW_MAPPING

#include "AppBase.h"
#include "VulkanImage.h"
#include "Light.h"
#include "Model.h"
#include "IBLResources.h"

// Pipelines
#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelinePBRShadowMapping.h"
#include "PipelineShadow.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineLightRender.h"
#include "PipelineImGui.h"

// STL
#include <vector>
#include <memory>

/*
Naive forward PBR
*/
class AppPBRShadowMapping final : AppBase
{
public:
	AppPBRShadowMapping();
	int MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();
	void DestroyResources();

private:
	std::unique_ptr<PipelineClear> clearPtr_;
	std::unique_ptr<PipelineSkybox> skyboxPtr_;
	std::unique_ptr<PipelineShadow> shadowPtr_;
	std::unique_ptr<PipelinePBRShadowMapping> pbrPtr_;
	std::unique_ptr<PipelineTonemap> tonemapPtr_;
	std::unique_ptr<PipelineFinish> finishPtr_;
	std::unique_ptr<PipelineResolveMS> resolveMSPtr_;
	std::unique_ptr<PipelineLightRender> lightPtr_;
	std::unique_ptr<PipelineImGui> imguiPtr_;

	float shadowNearPlane_;
	float shadowFarPlane_;
	uint32_t shadowMapSize_;
	VulkanImage shadowMap_;
	
	// PBR stuff
	//VulkanImage environmentCubemap_;
	//VulkanImage diffuseCubemap_;
	//VulkanImage specularCubemap_;
	//VulkanImage brdfLut_;
	std::unique_ptr<IBLResources> iblResources_;
	float cubemapMipmapCount_;

	std::unique_ptr<Model> sponzaModel_;
	std::unique_ptr<Model> tachikomaModel_;

	Lights lights_;

	ShadowMapUBO shadowUBO_;
};

#endif