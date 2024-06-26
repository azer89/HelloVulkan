#ifndef APP_PBR_BINDLESS_TEXTURES
#define APP_PBR_BINDLESS_TEXTURES

#include "AppBase.h"
#include "Scene.h"
#include "ResourcesLight.h"
#include "PipelineImGui.h"
#include "PipelinePBRBindless.h"
#include "PipelineLightRender.h"

#include <memory>

/*
Demo for bindless textures
*/
class AppPBRBindless final : AppBase
{
public:
	AppPBRBindless();
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();

private:
	PipelinePBRBindless* pbrPtr_{};
	PipelineLightRender* lightPtr_{};
	PipelineImGui* imguiPtr_{};

	std::unique_ptr<Scene> scene_{};
	ResourcesLight* resourcesLight_{};
};

#endif