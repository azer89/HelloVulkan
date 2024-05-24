#version 460 core
#extension GL_EXT_buffer_reference : require

#include <ModelUBO.glsl>
#include <ShadowMapping/UBO.glsl>
#include <Bindless/VertexData.glsl>
#include <Bindless/MeshData.glsl>
#include <Bindless/BDA.glsl>

layout(push_constant) uniform PC { BDA bda; };

layout(set = 0, binding = 0) uniform UBOBlock { ShadowUBO shadowUBO; };
layout(set = 0, binding = 1) readonly buffer ModelUBOs { ModelUBO modelUBOs []; };

void main()
{
	MeshData meshData = bda.meshReference.meshes[gl_BaseInstance];
	uint vOffset = meshData.vertexOffset;
	uint iOffset = meshData.indexOffset;
	uint vIndex = bda.indexReference.indices[iOffset + gl_VertexIndex] + vOffset;
	VertexData vertexData = bda.vertexReference.vertices[vIndex];
	mat4 model = modelUBOs[meshData.modelMatrixIndex].model;

	// Output
	gl_Position = shadowUBO.lightSpaceMatrix * model * vec4(vertexData.position, 1.0);
}