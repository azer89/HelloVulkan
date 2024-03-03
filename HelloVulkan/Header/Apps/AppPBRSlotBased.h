#ifndef APP_PBR_SLOT_BASED
#define APP_PBR_SLOT_BASED

#include "AppBase.h"
#include "Model.h"
#include "ResourcesLight.h"

// Pipelines
#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelinePBRSlotBased.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineLightRender.h"
#include "PipelineImGui.h"

// STL
#include <memory>

/*
Naive forward PBR
*/
class AppPBRSlotBased final : AppBase
{
public:
	AppPBRSlotBased();
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();
	void DestroyResources();

private:
	std::unique_ptr<PipelineClear> clearPtr_;
	std::unique_ptr<PipelineSkybox> skyboxPtr_;
	std::unique_ptr<PipelinePBRSlotBased> pbrPtr_;
	std::unique_ptr<PipelineTonemap> tonemapPtr_;
	std::unique_ptr<PipelineFinish> finishPtr_;
	std::unique_ptr<PipelineResolveMS> resolveMSPtr_;
	std::unique_ptr<PipelineLightRender> lightPtr_;
	std::unique_ptr<PipelineImGui> imguiPtr_;

	std::unique_ptr<ResourcesLight> resLights_;
	std::unique_ptr<Model> model_;

	float cubemapMipmapCount_;
	float modelRotation_;
};

#endif