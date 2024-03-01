#include "Scene.h"

#include "glm/glm.hpp"

#include <iostream>

Scene::Scene(VulkanContext& ctx, const std::vector<std::string>& modelFilenames)
{
	uint32_t vertexOffset = 0u;
	uint32_t indexOffset = 0u;
	for (const std::string& filename : modelFilenames)
	{
		Model m;
		m.LoadBindless(
			ctx,
			filename,
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
			meshDataArray_.emplace_back(models_[i].meshes_[j].GetMeshData(textureIndexOffset, i));
		}
		textureIndexOffset += models_[i].GetNumTextures();
	}
	VkDeviceSize meshDataBufferSize = static_cast<VkDeviceSize>(sizeof(MeshData) * meshDataArray_.size());
	meshDataBuffer_.CreateGPUOnlyBuffer(
		ctx, 
		meshDataBufferSize,
		meshDataArray_.data(), 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	
	// Vertices
	VkDeviceSize vertexBufferSize = static_cast<VkDeviceSize>(sizeof(VertexData) * vertices_.size());
	vertexBuffer_.CreateGPUOnlyBuffer(
		ctx,
		vertexBufferSize,
		vertices_.data(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	// Indices
	VkDeviceSize indexBufferSize = static_cast<VkDeviceSize>(sizeof(uint32_t) * indices_.size());
	indexBuffer_.CreateGPUOnlyBuffer(
		ctx,
		indexBufferSize,
		indices_.data(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	// ModelUBO
	// TODO Rename to SSBO
	const std::vector<ModelUBO> initModelUBOs(models_.size(), { .model = glm::mat4(1.0f) });
	VkDeviceSize modelUBOBufferSize = sizeof(ModelUBO) * models_.size();
	constexpr uint32_t frameCount = AppConfig::FrameOverlapCount;
	modelUBOBuffers_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		modelUBOBuffers_[i].CreateBuffer(ctx, modelUBOBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU);
		modelUBOBuffers_[i].UploadBufferData(ctx, initModelUBOs.data(), modelUBOBufferSize);
	}
}

void Scene::UpdateModelMatrix(VulkanContext& ctx, 
	ModelUBO modelUBO, 
	uint32_t modelIndex,
	uint32_t frameIndex)
{
	if (modelIndex < 0 || modelIndex >= models_.size())
	{
		std::cerr << "Cannot update ModelUBO because of invalid modelIndex " << modelIndex << "\n";
		return;
	}
	if (frameIndex < 0 || frameIndex >= AppConfig::FrameOverlapCount)
	{
		std::cerr << "Cannot update ModelUBO because of invalid frameIndex " << frameIndex << "\n";
		return;
	}
	modelUBOBuffers_[frameIndex].UploadOffsetBufferData(
		ctx,
		&modelUBO,
		sizeof(ModelUBO) * modelIndex,
		sizeof(ModelUBO));
}

void Scene::UpdateModelMatrix(VulkanContext& ctx,
	ModelUBO modelUBO,
	uint32_t modelIndex)
{
	for (uint32_t i = 0; i < AppConfig::FrameOverlapCount; ++i)
	{
		UpdateModelMatrix(ctx, modelUBO, modelIndex, i);
	}
}

// This is for descriptor indexing
std::vector<VkDescriptorImageInfo> Scene::GetImageInfos()
{
	std::vector<VkDescriptorImageInfo> textureInfoArray;
	//for (size_t i = 0; i < models_.size(); ++i)
	for (auto& model : models_)
	{
		//for (size_t j = 0; j < model.textureList_.size(); ++j)
		for (auto& texture : model.textureList_)
		{
			textureInfoArray.emplace_back(texture.GetDescriptorImageInfo());
		}
	}
	return textureInfoArray;
}

// This is for indirect draw
std::vector<uint32_t> Scene::GetMeshVertexCountArray()
{
	std::vector<uint32_t> vCountArray(GetMeshCount());
	size_t counter = 0;
	//for (size_t i = 0; i < models_.size(); ++i)
	for (auto& model : models_)
	{
		//for (size_t j = 0; j < models_[i].meshes_.size(); ++j)
		for (auto& mesh : model.meshes_)
		{
			// Note that we use the index count here
			uint32_t numIndices = static_cast<uint32_t>(mesh.GetNumIndices());
			vCountArray[counter++] = numIndices;
		}
	}
	return vCountArray;
}