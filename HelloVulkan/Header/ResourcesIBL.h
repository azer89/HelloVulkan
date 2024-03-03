#ifndef IBL_RESOURCES
#define IBL_RESOURCES

#include "VulkanContext.h"
#include "VulkanImage.h"

struct ResourcesIBL
{
public:
	VulkanImage environmentCubemap_;
	VulkanImage diffuseCubemap_;
	VulkanImage specularCubemap_;
	VulkanImage brdfLut_;

	ResourcesIBL() = default;
	~ResourcesIBL()
	{
		Destroy();
	}

	void Destroy()
	{
		environmentCubemap_.Destroy();
		diffuseCubemap_.Destroy();
		specularCubemap_.Destroy();
		brdfLut_.Destroy();
	}

	void SetDebugNames(VulkanContext& ctx)
	{
		environmentCubemap_.SetDebugName(ctx, "Environment_Cubemap");
		diffuseCubemap_.SetDebugName(ctx, "Diffuse_Cubemap");
		specularCubemap_.SetDebugName(ctx, "Specular_Cubemap");
		brdfLut_.SetDebugName(ctx, "BRDF_LUT");
	}
};

#endif