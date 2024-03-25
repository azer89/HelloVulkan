#ifndef RESOURCES_SHARED
#define RESOURCES_SHARED

#include "VulkanContext.h"
#include "VulkanImage.h"
#include "ResourcesBase.h"

struct ResourcesShared : ResourcesBase
{
public:
	ResourcesShared();
	~ResourcesShared();

	void Create(VulkanContext& ctx);
	void Destroy() override;

public:
	VulkanImage multiSampledColorImage_;
	VulkanImage singleSampledColorImage_;
	VulkanImage depthImage_;
};

#endif