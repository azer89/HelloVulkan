#ifndef G_BUFFER
#define G_BUFFER

#include "glm/glm.hpp"

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "ResourcesBase.h"

struct ResourcesGBuffer : ResourcesBase
{
public:
	ResourcesGBuffer();
	~ResourcesGBuffer();

	void Create(VulkanContext& ctx);
	void Destroy() override;

	void UpdateFromUIData(VulkanContext& ctx, UIData& uiData) override
	{
	}
};

#endif