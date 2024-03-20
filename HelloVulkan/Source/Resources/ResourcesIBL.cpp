#include "ResourcesIBL.h"
#include "PipelineEquirect2Cube.h"
#include "PipelineCubeFilter.h"
#include "PipelineBRDFLUT.h"
#include "VulkanUtility.h"

ResourcesIBL::ResourcesIBL(VulkanContext& ctx, const std::string& hdrFile)
{
	Create(ctx, hdrFile);
	SetDebugNames(ctx);
}

ResourcesIBL::~ResourcesIBL()
{
	Destroy();
}

void ResourcesIBL::Create(VulkanContext& ctx, const std::string& hdrFile)
{
	// Create a cubemap from the input HDR
	{
		PipelineEquirect2Cube e2c(ctx, hdrFile);
		e2c.OffscreenRender(ctx, &environmentCubemap_);
	}

	// Cube filtering
	{
		PipelineCubeFilter cubeFilter(ctx, &(environmentCubemap_));
		cubeFilter.OffscreenRender(ctx, &diffuseCubemap_, CubeFilterType::Diffuse);
		cubeFilter.OffscreenRender(ctx, &specularCubemap_, CubeFilterType::Specular);
	}

	// BRDF look up table
	{
		PipelineBRDFLUT brdfLUTCompute(ctx);
		brdfLUTCompute.CreateLUT(ctx, &brdfLut_);
	}

	cubemapMipmapCount_ = static_cast<float>(Utility::MipMapCount(IBLConfig::InputCubeSideLength));
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