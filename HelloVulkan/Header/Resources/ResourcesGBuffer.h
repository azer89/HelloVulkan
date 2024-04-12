#ifndef G_BUFFER
#define G_BUFFER

#include "glm/glm.hpp"

#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "ResourcesBase.h"

struct ResourcesGBuffer : ResourcesBase
{
public:
	ResourcesGBuffer();
	~ResourcesGBuffer();

	void Create(VulkanContext& ctx);
	void Destroy() override;
	void OnWindowResized(VulkanContext& ctx) override;

	void UpdateFromUIData(VulkanContext& ctx, UIData& uiData) override
	{
	}

private:
	void CreateSampler(VulkanContext& ctx, VkSampler* sampler);

public:
	VulkanImage position_;
	VulkanImage normal_;
	VulkanImage noise_;
	VulkanImage ssao_;
	VulkanBuffer kernel_;

	// Needed for depth test
	VulkanImage depth_;

private:
	std::vector<glm::vec3> ssaoKernel_ = {};
};

#endif