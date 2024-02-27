#version 460 core

//layout(location = 0) in vec4 inPosition;
//layout(location = 1) in vec4 inNormal;
//layout(location = 2) in vec4 inUV;

layout(set = 0, binding = 0)
#include <ShadowMapping//UBO.glsl>

//layout(set = 0, binding = 1) uniform ModelUBO
//{
//	mat4 model;
//};

// SSBO
struct ModelUBO
{
	mat4 model;
};
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

	gl_Position = shadowUBO.lightSpaceMatrix * model * vertexData.position;
	//gl_Position = shadowUBO.lightSpaceMatrix * model * vec4(inPosition.xyz, 1.0);
}