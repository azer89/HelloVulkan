#ifndef IBL_RESOURCES
#define IBL_RESOURCES

#include "VulkanContext.h"
#include "VulkanImage.h"

struct ResourcesIBL
{
public:
	ResourcesIBL();
	~ResourcesIBL();

	void Destroy();
	void SetDebugNames(VulkanContext& ctx);

public:
	VulkanImage environmentCubemap_;
	VulkanImage diffuseCubemap_;
	VulkanImage specularCubemap_;
	VulkanImage brdfLut_;

};

#endif