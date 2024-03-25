#ifndef APP_PBR_SLOT_BASED
#define APP_PBR_SLOT_BASED

#include "AppBase.h"
#include "Model.h"
#include "PipelineImGui.h"
#include "ResourcesLight.h"

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

private:
	PipelineImGui* imguiPtr_ = nullptr;
	ResourcesLight* resourcesLights_ = nullptr;
	std::unique_ptr<Model> model_ = nullptr;
};

#endif