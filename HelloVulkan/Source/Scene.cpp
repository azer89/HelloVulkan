#include "Scene.h"

#include "glm/glm.hpp"

#include <iostream>

Scene::Scene(VulkanContext& ctx, const std::vector<std::string>& modelFiles)
{
	uint32_t vertexOffset = 0u;
	uint32_t indexOffset = 0u;
	for (const std::string& file : modelFiles)
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
	for (auto& buffer : modelUBOBuffers_)
	{
		buffer.Destroy();
	}
	for (auto& model : models_)
	{
		model.Destroy();
	}
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
	std::vector<ModelUBO> initModelUBOs(models_.size(), { .model = glm::mat4(1.0f) });
	const uint32_t frameCount = AppConfig::FrameOverlapCount;
	modelUBOBufferSize_ = sizeof(ModelUBO) * models_.size();
	modelUBOBuffers_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		modelUBOBuffers_[i].CreateBuffer(ctx, modelUBOBufferSize_,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU);

		modelUBOBuffers_[i].UploadBufferData(ctx, initModelUBOs.data(), modelUBOBufferSize_);
	}
}

// This is for descriptor indexing
std::vector<VkDescriptorImageInfo> Scene::GetImageInfos()
{
	std::vector<VkDescriptorImageInfo> textureInfoArray;
	for (size_t i = 0; i < models_.size(); ++i)
	{
		for (size_t j = 0; j < models_[i].textureList_.size(); ++j)
		{
			textureInfoArray.emplace_back(models_[i].textureList_[j].GetDescriptorImageInfo());
		}
	}
	return textureInfoArray;
}

// This is for indirect draw
std::vector<uint32_t> Scene::GetMeshVertexCountArray()
{
	std::vector<uint32_t> vCountArray(GetMeshCount());
	size_t counter = 0;
	for (size_t i = 0; i < models_.size(); ++i)
	{
		for (size_t j = 0; j < models_[i].meshes_.size(); ++j)
		{
			// Note that we use the index count here
			uint32_t numIndices = static_cast<uint32_t>(models_[i].meshes_[j].indices_.size());
			vCountArray[counter++] = numIndices;
			//std::cout << models_[i].meshes_[j].indices_.size() << "\n";
		}
	}
	return vCountArray;
}
