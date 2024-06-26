#ifndef IBL_RESOURCES
#define IBL_RESOURCES

#include "VulkanContext.h"
#include "ResourcesBase.h"
#include "VulkanImage.h"

struct ResourcesIBL : ResourcesBase
{
public:
	ResourcesIBL(VulkanContext& ctx, const std::string& hdrFile);
	~ResourcesIBL();
	void Destroy() override;

private:
	void Create(VulkanContext& ctx, const std::string& hdrFile);
	void SetDebugNames(VulkanContext& ctx);

public:
	float cubemapMipmapCount_ = 0.0f;
	VulkanImage environmentCubemap_{};
	VulkanImage diffuseCubemap_{};
	VulkanImage specularCubemap_{};
	VulkanImage brdfLut_{};
};

#endif