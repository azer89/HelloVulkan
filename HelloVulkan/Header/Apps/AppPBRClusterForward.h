#ifndef APP_PBR_CLUSTER_FORWARD
#define APP_PBR_CLUSTER_FORWARD

#include "AppBase.h"
#include "VulkanImage.h"
#include "Light.h"
#include "Model.h"
#include "ResourcesIBL.h"

// Pipelines
#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineLightRender.h"
#include "PipelineImGui.h"

// Clustered forward
#include "PipelinePBRClusterForward.h"
#include "PipelineAABBGenerator.h"
#include "PipelineLightCulling.h"
#include "ResourcesClusterForward.h"

// STL
#include <vector>
#include <memory>

/*
Clustered forward PBR
*/
class AppPBRClusterForward final : AppBase
{
public:
	AppPBRClusterForward();
	int MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();
	void DestroyResources();

private:
	std::unique_ptr<PipelineClear> clearPtr_;
	std::unique_ptr<PipelineSkybox> skyboxPtr_;

	std::unique_ptr<PipelineTonemap> tonemapPtr_;
	std::unique_ptr<PipelineFinish> finishPtr_;
	std::unique_ptr<PipelineResolveMS> resolveMSPtr_;
	std::unique_ptr<PipelineLightRender> lightPtr_;
	std::unique_ptr<PipelineImGui> imguiPtr_;

	std::unique_ptr<PipelinePBRClusterForward> pbrPtr_;
	std::unique_ptr<PipelineAABBGenerator> aabbPtr_;
	std::unique_ptr<PipelineLightCulling> lightCullPtr_;

	// Buffers for clustered forward shading
	std::unique_ptr<ResourcesClusterForward> resCF_; 

	std::unique_ptr<Model> model_;

	std::unique_ptr<Lights> lights_;

	float cubemapMipmapCount_;
};

#endif