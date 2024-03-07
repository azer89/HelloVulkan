#ifndef RESOURCES_SHADOW
#define RESOURCES_SHADOW

#include "VulkanContext.h"
#include "VulkanImage.h"
#include "Configs.h"

#include <array>

struct ResourcesShadow
{
public:
	ResourcesShadow();
	~ResourcesShadow();

	void CreateSingleShadowMap(VulkanContext& ctx);
	void CreateCascadeShadowMap(VulkanContext& ctx);
	void Destroy();

public:
	VulkanImage shadowMap_;
	std::array<VkImageView, ShadowConfig::CascadeCount> views_;

private:
	VkDevice device_;
};

#endif