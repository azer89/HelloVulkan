#ifndef APP_PBR
#define APP_PBR

#include "AppBase.h"
#include "RendererSkybox.h"
#include "RendererClear.h"
#include "RendererFinish.h"
#include "RendererPBR.h"
#include "RendererTonemap.h"
#include "RendererResolveMS.h"
#include "RendererLight.h"
#include "RendererImGui.h"
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

	void Init();
	void InitLights();
	void DestroyResources();

private:
	std::unique_ptr<RendererClear> clearPtr_;
	std::unique_ptr<RendererSkybox> skyboxPtr_;
	std::unique_ptr<RendererPBR> pbrPtr_;
	std::unique_ptr<RendererTonemap> tonemapPtr_;
	std::unique_ptr<RendererFinish> finishPtr_;
	std::unique_ptr<RendererResolveMS> resolveMSPtr_;
	std::unique_ptr<RendererLight> lightPtr_;
	std::unique_ptr<RendererImGui> imguiPtr_;

	// PBR stuff
	// TODO change to unique ptrs
	VulkanImage environmentCubemap_;
	VulkanImage diffuseCubemap_;
	VulkanImage specularCubemap_;
	VulkanImage brdfLut_;

	float modelRotation_;
	std::unique_ptr<Model> model_;

	Lights lights_;
};

#endif