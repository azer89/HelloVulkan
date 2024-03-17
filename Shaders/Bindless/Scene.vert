#version 460 core
#extension GL_EXT_buffer_reference : require

// Bindless/Scene.vert
// Bindless textures

#include <ModelUBO.glsl>
#include <CameraUBO.glsl>
#include <Bindless/VertexData.glsl>
#include <Bindless/MeshData.glsl>
#include <Bindless/VIM.glsl>

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec3 normal;
layout(location = 3) out flat uint meshIndex;

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; }; // UBO
layout(set = 0, binding = 1) readonly buffer ModelUBOs { ModelUBO modelUBOs[]; }; // SSBO
layout(set = 0, binding = 2) uniform VIMBlock { VIM vim; }; // UBO

void main()
{
	MeshData meshData = vim.meshReference.meshes[gl_BaseInstance];
	uint vOffset = meshData.vertexOffset;
	uint iOffset = meshData.indexOffset;
	uint vIndex = vim.indexReference.indices[iOffset + gl_VertexIndex] + vOffset;
	VertexData vertexData = vim.vertexReference.vertices[vIndex];
	mat4 model = modelUBOs[meshData.modelMatrixIndex].model;
	mat3 normalMatrix = transpose(inverse(mat3(model)));

	// Output
	worldPos = (model * vec4(vertexData.position, 1.0)).xyz;
	texCoord = vec2(vertexData.uvX,vertexData.uvY);
	normal = normalMatrix * vertexData.normal;
	meshIndex = gl_BaseInstance;
	gl_Position =  camUBO.projection * camUBO.view * vec4(worldPos, 1.0);
}
