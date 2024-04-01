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

	void UpdateFromUIData(VulkanContext& ctx, UIData& uiData) override
	{
		shadowUBO_.shadowMinBias = uiData.shadowMinBias_;
		shadowUBO_.shadowMaxBias = uiData.shadowMaxBias_;
		shadowNearPlane_ = uiData.shadowNearPlane_;
		shadowFarPlane_ = uiData.shadowFarPlane_;
		orthoSize_ = uiData.shadowOrthoSize_;
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