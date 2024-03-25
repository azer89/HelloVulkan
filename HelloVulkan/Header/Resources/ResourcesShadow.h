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

	void GetUpdateFromInputContext(VulkanContext& ctx, InputContext& inputContext) override
	{
		shadowUBO_.shadowMinBias = inputContext.shadowMinBias_;
		shadowUBO_.shadowMaxBias = inputContext.shadowMaxBias_;
		shadowNearPlane_ = inputContext.shadowNearPlane_;
		shadowFarPlane_ = inputContext.shadowFarPlane_;
		orthoSize_ = inputContext.shadowOrthoSize_;
	}

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