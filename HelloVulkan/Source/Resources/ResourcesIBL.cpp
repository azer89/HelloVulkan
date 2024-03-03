#include "ResourcesIBL.h"

ResourcesIBL::ResourcesIBL()
{
}

ResourcesIBL::~ResourcesIBL()
{
	Destroy();
}

void ResourcesIBL::Destroy()
{
	environmentCubemap_.Destroy();
	diffuseCubemap_.Destroy();
	specularCubemap_.Destroy();
	brdfLut_.Destroy();
}

void ResourcesIBL::SetDebugNames(VulkanContext& ctx)
{
	environmentCubemap_.SetDebugName(ctx, "Environment_Cubemap");
	diffuseCubemap_.SetDebugName(ctx, "Diffuse_Cubemap");
	specularCubemap_.SetDebugName(ctx, "Specular_Cubemap");
	brdfLut_.SetDebugName(ctx, "BRDF_LUT");
}