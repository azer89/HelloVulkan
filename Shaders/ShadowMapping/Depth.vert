#version 460 core

#include <ModelUBO.glsl>

layout(set = 0, binding = 0)
#include <ShadowMapping//UBO.glsl>

// SSBO
layout(set = 0, binding = 1) readonly buffer ModelUBOs { ModelUBO modelUBOs []; };

// SSBO
#include <Bindless//VertexData.glsl>
layout(set = 0, binding = 2) readonly buffer Vertices { VertexData vertices []; };

// SSBO
layout(set = 0, binding = 3) readonly buffer Indices { uint indices []; };

// SSBO
#include <Bindless//MeshData.glsl>
layout(set = 0, binding = 4) readonly buffer Meshes { MeshData meshes []; };

void main()
{
	MeshData meshData = meshes[gl_BaseInstance];
	uint vOffset = meshData.vertexOffset;
	uint iOffset = meshData.indexOffset;
	uint vIndex = indices[iOffset + gl_VertexIndex] + vOffset;
	VertexData vertexData = vertices[vIndex];
	mat4 model = modelUBOs[meshData.modelMatrixIndex].model;

	// Output
	gl_Position = shadowUBO.lightSpaceMatrix * model * vertexData.position;
}