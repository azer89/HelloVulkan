#ifndef APP_PBR
#define APP_PBR

#include "AppBase.h"
#include "RendererSkybox.h"
#include "RendererClear.h"
#include "RendererFinish.h"
#include "RendererPBR.h"
#include "VulkanImage.h"
#include "Model.h"

#include <vector>
#include <memory>

class AppPBR : AppBase
{
public:
	AppPBR();
	int MainLoop() override;
	void UpdateUBO(uint32_t imageIndex) override;

	void Init();
	void DestroyResources();

private:
	std::vector<RendererBase*> renderers_;

	std::unique_ptr<RendererSkybox> skyboxPtr_;
	std::unique_ptr<RendererClear> clearPtr_;
	std::unique_ptr<RendererFinish> finishPtr_;
	std::unique_ptr<RendererPBR> pbrPtr_;

	// Cubemap generated from HDR
	VulkanImage environmentCubemap_;

	// PBR stuff
	VulkanImage diffuseCubemap_;
	VulkanImage lutTexture_;
	VulkanImage specularCubemap_;

	VulkanImage depthImage_;

	float modelRotation_;
	std::unique_ptr<Model> model_;
};

#endif