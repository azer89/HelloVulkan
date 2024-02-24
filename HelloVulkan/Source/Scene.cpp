#include "Scene.h"

#include "glm/glm.hpp"

Scene::Scene(VulkanContext& ctx,
	std::vector<std::string> modelFiles)
{
	uint32_t vertexOffset = 0u;
	uint32_t indexOffset = 0u;
	for (std::string& file : modelFiles)
	{
		Model m;
		m.LoadBindless(
			ctx,
			file,
			vertices_,
			indices_,
			vertexOffset,
			indexOffset);
		models_.push_back(m);
	}
	CreateBindlessResources(ctx);
}

Scene::~Scene()
{
	meshDataBuffer_.Destroy();
	vertexBuffer_.Destroy();
	indexBuffer_.Destroy();
	modelUBOBuffer_.Destroy();
}

// TODO Create GPU only buffers
void Scene::CreateBindlessResources(VulkanContext& ctx)
{
	// Mesh data
	uint32_t textureIndexOffset = 0u;
	for (size_t i = 0; i < models_.size(); ++i)
	{
		for (size_t j = 0; j < models_[i].meshes_.size(); ++j)
		{
			meshDataArray_.emplace_back(models_[i].meshes_[j].GetMeshData(textureIndexOffset));
		}
		textureIndexOffset += models_[i].GetNumTextures();
	}
	meshCount_ += meshDataArray_.size();
	meshDataBufferSize_ = static_cast<VkDeviceSize>(sizeof(MeshData) * meshDataArray_.size());
	meshDataBuffer_.CreateBuffer(
		ctx,
		meshDataBufferSize_,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU, // TODO GPU only
		0
	);
	meshDataBuffer_.UploadBufferData(ctx, meshDataArray_.data(), meshDataBufferSize_);

	// Vertices
	vertexBufferSize_ = static_cast<VkDeviceSize>(sizeof(VertexData) * vertices_.size());
	vertexBuffer_.CreateBuffer(
		ctx,
		vertexBufferSize_,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU, // TODO GPU only
		0
	);
	vertexBuffer_.UploadBufferData(ctx, vertices_.data(), vertexBufferSize_);

	// Indices
	indexBufferSize_ = static_cast<VkDeviceSize>(sizeof(uint32_t) * indices_.size());
	indexBuffer_.CreateBuffer(
		ctx,
		indexBufferSize_,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU, // TODO GPU only
		0
	);
	indexBuffer_.UploadBufferData(ctx, indices_.data(), indexBufferSize_);

	// ModelUBO
	modelUBOBufferSize_ = sizeof(ModelUBO) * models_.size();
	modelUBOBuffer_.CreateBuffer(ctx, modelUBOBufferSize_,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	std::vector<ModelUBO> initModelUBOs(models_.size());
	for (size_t i = 0; i < models_.size(); ++i)
	{
		initModelUBOs[i] = {.model = glm::mat4(1.0f)};
	}
	modelUBOBuffer_.UploadBufferData(ctx, initModelUBOs.data(), modelUBOBufferSize_);
}