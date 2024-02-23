#ifndef SCENE
#define SCENE

#include "Model.h"

#include <vector>
#include <string>

class Scene
{
public:
	Scene(VulkanContext& ctx,
		std::vector<std::string> modelFiles)
	{

	}

	~Scene()
	{
	}

public:
	// Bindless rendering
	std::vector<MeshData> meshDataArray_;
	std::vector<VertexData> vertices_;
	std::vector<uint32_t> indices_;
	VkDeviceSize meshDataBufferSize_;
	VkDeviceSize vertexBufferSize_;
	VkDeviceSize indexBufferSize_;
	VulkanBuffer meshDataBuffer_;
	VulkanBuffer vertexBuffer_;
	VulkanBuffer indexBuffer_;

private:


};

#endif