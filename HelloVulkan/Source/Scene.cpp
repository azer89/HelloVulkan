#include "Scene.h"

#include "glm/glm.hpp"

#include <iostream>

Scene::Scene(VulkanContext& ctx, const std::vector<std::string>& modelFilenames, bool supportDeviceAddress) :
	supportDeviceAddress_(supportDeviceAddress)
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
	CreateBindlessTextureResources(ctx);
}

Scene::~Scene()
{
	meshDataBuffer_.Destroy();
	vertexBuffer_.Destroy();
	indexBuffer_.Destroy();
	transformedBoundingBoxBuffer_.Destroy();
	for (auto& buffer : modelSSBOBuffers_)
	{
		buffer.Destroy();
	}
	for (auto& buffer : indirectBuffers_)
	{
		buffer.Destroy();
	}
	for (auto& model : models_)
	{
		model.Destroy();
	}
}

VIM Scene::GetVIM() const
{
	return
	{
		.vertexBufferAddress = vertexBuffer_.deviceAddress_,
		.indexBufferAddress = indexBuffer_.deviceAddress_,
		.meshDataBufferAddress = meshDataBuffer_.deviceAddress_
	};
}

// TODO Create GPU only buffers
void Scene::CreateBindlessTextureResources(VulkanContext& ctx)
{
	// Support for bindless rendering
	VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (supportDeviceAddress_)
	{
		bufferUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}

	// Mesh data
	uint32_t textureIndexOffset = 0u;
	for (size_t i = 0; i < models_.size(); ++i)
	{
		for (size_t j = 0; j < models_[i].meshes_.size(); ++j)
		{
			meshDataArray_.emplace_back(models_[i].meshes_[j].GetMeshData(textureIndexOffset, static_cast<uint32_t>(i)));
		}
		textureIndexOffset += models_[i].GetTextureCount();
	}
	const VkDeviceSize meshDataBufferSize = static_cast<VkDeviceSize>(sizeof(MeshData) * meshDataArray_.size());
	meshDataBuffer_.CreateGPUOnlyBuffer(
		ctx, 
		meshDataBufferSize,
		meshDataArray_.data(), 
		bufferUsage);

	// Vertices
	const VkDeviceSize vertexBufferSize = static_cast<VkDeviceSize>(sizeof(VertexData) * vertices_.size());
	vertexBuffer_.CreateGPUOnlyBuffer(
		ctx,
		vertexBufferSize,
		vertices_.data(),
		bufferUsage);

	// Indices
	const VkDeviceSize indexBufferSize = static_cast<VkDeviceSize>(sizeof(uint32_t) * indices_.size());
	indexBuffer_.CreateGPUOnlyBuffer(
		ctx,
		indexBufferSize,
		indices_.data(),
		bufferUsage);

	// ModelUBO which is actually an SSBO
	modelUBOs_ = std::vector<ModelUBO>(models_.size(), { .model = glm::mat4(1.0f) });
	const VkDeviceSize modelSSBOBufferSize = sizeof(ModelUBO) * models_.size();
	constexpr uint32_t frameCount = AppConfig::FrameCount;
	modelSSBOBuffers_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		modelSSBOBuffers_[i].CreateBuffer(ctx, modelSSBOBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU);
		modelSSBOBuffers_[i].UploadBufferData(ctx, modelUBOs_.data(), modelSSBOBufferSize);
	}

	BuildModelToMeshDataMapping();

	// Bounding boxes
	BuildBoundingBoxes(ctx);

	// Indirect buffers
	CreateIndirectBuffers(ctx, indirectBuffers_);
}

void Scene::BuildModelToMeshDataMapping()
{
	modelToMeshMap_.resize(models_.size());
	int iter = 0;
	for (size_t i = 0; i < models_.size(); ++i)
	{
		size_t meshCount = models_[i].GetMeshCount();
		modelToMeshMap_[i].resize(meshCount);
		for (size_t j = 0; j < meshCount; ++j)
		{
			modelToMeshMap_[i][j] = iter++;
		}
	}
}

void Scene::BuildBoundingBoxes(VulkanContext& ctx)
{
	// Create bounding boxes
	originalBoundingBoxes_.resize(meshDataArray_.size());
	transformedBoundingBoxes_.resize(meshDataArray_.size());
	size_t iter = 0;
	for (Model& model : models_)
	{
		glm::vec3 vmin(std::numeric_limits<float>::max());
		glm::vec3 vmax(std::numeric_limits<float>::lowest());

		for (Mesh& mesh : model.meshes_)
		{
			uint32_t vertexStart = mesh.GetVertexOffset();
			uint32_t vertexEnd = vertexStart + mesh.GetVertexCount();
			for (uint32_t i = vertexStart; i < vertexEnd; ++i)
			{
				const glm::vec3& v = vertices_[i].position;
				vmin = glm::min(vmin, v);
				vmax = glm::max(vmax, v);
			}

			originalBoundingBoxes_[iter].min_ = glm::vec4(vmin, 1.0);
			originalBoundingBoxes_[iter].max_ = glm::vec4(vmax, 1.0);

			const MeshData& mData = meshDataArray_[iter];
			const glm::mat4& mat = modelUBOs_[mData.modelIndex].model;
			transformedBoundingBoxes_[iter] = originalBoundingBoxes_[iter].GetTransformed(mat);

			++iter;
		}
	}

	// Buffer
	VkDeviceSize bufferSize = static_cast<VkDeviceSize>(transformedBoundingBoxes_.size() * sizeof(BoundingBox));
	transformedBoundingBoxBuffer_.CreateBuffer(ctx, 
		bufferSize, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	transformedBoundingBoxBuffer_.UploadBufferData(ctx, transformedBoundingBoxes_.data(), bufferSize);
}

void Scene::CreateIndirectBuffers(
	VulkanContext& ctx,
	std::vector<VulkanBuffer>& indirectBuffers)
{
	const uint32_t meshSize = GetMeshCount();
	const uint32_t indirectDataSize = meshSize * sizeof(VkDrawIndirectCommand);
	constexpr size_t numFrames = AppConfig::FrameCount;
	const std::vector<uint32_t> meshVertexCountArray = GetMeshVertexCountArray();

	indirectBuffers.resize(numFrames);
	for (size_t i = 0; i < numFrames; ++i)
	{
		indirectBuffers[i].CreateIndirectBuffer(ctx, indirectDataSize); // Create
		VkDrawIndirectCommand* data = indirectBuffers[i].MapIndirectBuffer(); // Map

		for (uint32_t j = 0; j < meshSize; ++j)
		{
			data[j] =
			{
				.vertexCount = static_cast<uint32_t>(meshVertexCountArray[j]),
				.instanceCount = 1u,
				.firstVertex = 0,
				.firstInstance = j
			};
		}
		indirectBuffers[i].UnmapIndirectBuffer(); // Unmap
	}
}

void Scene::UpdateModelMatrix(VulkanContext& ctx,
	const ModelUBO& modelUBO,
	uint32_t modelIndex)
{
	if (modelIndex < 0 || modelIndex >= models_.size())
	{
		std::cerr << "Cannot update ModelUBO because of invalid modelIndex " << modelIndex << "\n";
		return;
	}

	// Update transformation matrix
	modelUBOs_[modelIndex] = modelUBO;

	// Update SSBO
	for (uint32_t i = 0; i < AppConfig::FrameCount; ++i)
	{
		modelSSBOBuffers_[i].UploadOffsetBufferData(
			ctx,
			&modelUBO,
			sizeof(ModelUBO) * modelIndex,
			sizeof(ModelUBO));
	}

	// Update bounding boxes
	std::vector<int>& meshIndices = modelToMeshMap_[modelIndex];
	if (meshIndices.size() > 0)
	{
		for (int i : meshIndices)
		{
			transformedBoundingBoxes_[i] = originalBoundingBoxes_[i].GetTransformed(modelUBO.model);
		}

		// Update bounding box buffers
		int firstIndex = meshIndices[0];
		transformedBoundingBoxBuffer_.UploadOffsetBufferData(
			ctx,
			transformedBoundingBoxes_.data() + firstIndex,
			sizeof(BoundingBox) * firstIndex,
			sizeof(BoundingBox) * meshIndices.size());
	}
}

// This is for descriptor indexing
std::vector<VkDescriptorImageInfo> Scene::GetImageInfos() const
{
	std::vector<VkDescriptorImageInfo> textureInfoArray;
	for (auto& model : models_)
	{
		for (auto& texture : model.textureList_)
		{
			textureInfoArray.emplace_back(texture.GetDescriptorImageInfo());
		}
	}
	return textureInfoArray;
}

// This is for indirect draw
std::vector<uint32_t> Scene::GetMeshVertexCountArray() const
{
	std::vector<uint32_t> vCountArray(GetMeshCount());
	size_t counter = 0;
	for (auto& model : models_)
	{
		for (auto& mesh : model.meshes_)
		{
			// Note that we use the index count here
			uint32_t indexCount = static_cast<uint32_t>(mesh.GetIndexCount());
			vCountArray[counter++] = indexCount;
		}
	}
	return vCountArray;
}