#ifndef APP_PBR
#define APP_PBR

#include "AppBase.h"
#include "VulkanImage.h"
#include "Light.h"
#include "Model.h"
#include "Scene.h"
#include "IBLResources.h"

// Pipelines
#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelinePBRBindless.h"
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
class AppPBR final : AppBase
{
public:
	AppPBR();
	int MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();
	void DestroyResources();

private:
	std::unique_ptr<PipelineClear> clearPtr_;
	std::unique_ptr<PipelineSkybox> skyboxPtr_;
	std::unique_ptr<PipelinePBRBindless> pbrPtr_;
	std::unique_ptr<PipelineTonemap> tonemapPtr_;
	std::unique_ptr<PipelineFinish> finishPtr_;
	std::unique_ptr<PipelineResolveMS> resolveMSPtr_;
	std::unique_ptr<PipelineLightRender> lightPtr_;
	std::unique_ptr<PipelineImGui> imguiPtr_;

	std::unique_ptr<Scene> scene_;
	Lights lights_;

	float cubemapMipmapCount_;
	float modelRotation_;
};

#endif