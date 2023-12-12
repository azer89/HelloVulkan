#ifndef RENDERER_CUBE
#define RENDERER_CUBE

#include "RendererBase.h"
#include "VulkanTexture.h"
#include "Bitmap.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

class RendererCube : public RendererBase
{
public:
	RendererCube(VulkanDevice& vkDev, VulkanImage inDepthTexture, const char* textureFile);
	virtual ~RendererCube();

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

	void UpdateUniformBuffer(VulkanDevice& vkDev, uint32_t currentImage, const glm::mat4& m);

private:
	VulkanTexture texture;

	bool CreateDescriptorSet(VulkanDevice& vkDev);

	bool CreateCubeTextureImage(
		VulkanDevice& vkDev,
		const char* filename,
		VkImage& textureImage,
		VkDeviceMemory& textureImageMemory,
		uint32_t* width = nullptr,
		uint32_t* height = nullptr);

	Bitmap ConvertEquirectangularMapToVerticalCross(const Bitmap& b);

	Bitmap ConvertVerticalCrossToCubeMapFaces(const Bitmap& b);

	Bitmap ConvertEquirectangularMapToCubeMapFaces(const Bitmap& b);

	void ConvolveDiffuse(
		const glm::vec3* data, 
		int srcW, 
		int srcH, 
		int dstW, 
		int dstH, 
		glm::vec3* output, 
		int numMonteCarloSamples);

	glm::vec3 FaceCoordsToXYZ(int i, int j, int faceID, int faceSize);
};

#endif
