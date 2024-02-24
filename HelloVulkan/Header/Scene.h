#ifndef SCENE
#define SCENE

#include "Model.h"
#include "UBO.h"

#include <vector>
#include <string>

class Scene
{
public:
	Scene(VulkanContext& ctx, std::vector<std::string> modelFiles);
	~Scene();

private:
	void CreateBindlessResources(VulkanContext& ctx);

public:
	std::vector<MeshData> meshDataArray_ = {};
	VulkanBuffer meshDataBuffer_;
	VkDeviceSize meshDataBufferSize_;

	std::vector<VertexData> vertices_ = {};
	VulkanBuffer vertexBuffer_;
	VkDeviceSize vertexBufferSize_;

	std::vector<uint32_t> indices_ = {};
	VulkanBuffer indexBuffer_;
	VkDeviceSize indexBufferSize_;

	VulkanBuffer modelUBOBuffer_;
	VkDeviceSize modelUBOBufferSize_;

	uint32_t meshCount_;

	std::vector<Model> models_ = {};
};

#endif