#ifndef APP_SIMPLE_RAYTRACING
#define APP_SIMPLE_RAYTRACING

#include "AppBase.h"
#include "VulkanImage.h"
#include "Model.h"

// Pipelines
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelineImGui.h"
#include "PipelineSimpleRaytracing.h"

// STL
#include <vector>
#include <memory>

class AppSimpleRaytracing final : AppBase
{
public:
	AppSimpleRaytracing();

	int MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void DestroyResources();

private:
	std::unique_ptr<PipelineClear> clearPtr_;
	std::unique_ptr<PipelineFinish> finishPtr_;
	std::unique_ptr<PipelineImGui> imguiPtr_;
	std::unique_ptr<PipelineSimpleRaytracing> rtxPtr_;
};

#endif