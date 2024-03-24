#ifndef RESOURCES_SHADOW
#define RESOURCES_SHADOW

#include "VulkanContext.h"
#include "VulkanImage.h"
#include "ResourcesBase.h"
#include "UBOs.h"

struct ResourcesShadow : ResourcesBase
{
public:
	ResourcesShadow();
	~ResourcesShadow();

	void CreateSingleShadowMap(VulkanContext& ctx);
	void Destroy() override;

public:
	VulkanImage shadowMap_;

	ShadowMapUBO shadowUBO_;
	float shadowNearPlane_;
	float shadowFarPlane_;
	float orthoSize_;

private:
	VkDevice device_;
};

#endif