#ifndef RESOURCES_SHADOW
#define RESOURCES_SHADOW

#include "VulkanContext.h"
#include "VulkanImage.h"
#include "UBOs.h"

struct ResourcesShadow
{
public:
	ResourcesShadow();
	~ResourcesShadow();

	void CreateSingleShadowMap(VulkanContext& ctx);
	void Destroy();

public:
	VulkanImage shadowMap_;

	ShadowMapUBO shadowUBO_;

private:
	VkDevice device_;
};

#endif