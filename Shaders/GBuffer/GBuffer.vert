#version 460 core
#extension GL_EXT_buffer_reference : require

#include <ModelUBO.glsl>
#include <CameraUBO.glsl>
#include <Bindless/VertexData.glsl>
#include <Bindless/MeshData.glsl>
#include <Bindless/BDA.glsl>

layout(location = 0) out vec3 viewPos;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec2 texCoord;
layout(location = 4) out flat uint meshIndex;

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; }; // UBO
layout(set = 0, binding = 1) readonly buffer ModelUBOs { ModelUBO modelUBOs[]; }; // SSBO
layout(set = 0, binding = 2) uniform BDABlock { BDA bda; }; // UBO

void main()
{
	MeshData meshData = bda.meshReference.meshes[gl_BaseInstance];
	uint vOffset = meshData.vertexOffset;
	uint iOffset = meshData.indexOffset;
	uint vIndex = bda.indexReference.indices[iOffset + gl_VertexIndex] + vOffset;
	VertexData vertexData = bda.vertexReference.vertices[vIndex];
	mat4 model = modelUBOs[meshData.modelMatrixIndex].model;
	mat3 normalMatrix = transpose(inverse(mat3(model)));

	// Output
	vec4 worldPos4 = model * vec4(vertexData.position, 1.0);
	vec4 viewPos4 = camUBO.view * worldPos4;
	viewPos = viewPos4.xyz;
	normal = normalMatrix * vertexData.normal;
	texCoord = vec2(vertexData.uvX, vertexData.uvY);
	meshIndex = gl_BaseInstance;
	vec4 fragPos4 = camUBO.projection * viewPos4;
	fragPos = fragPos.xyz;
	gl_Position =  fragPos4;
}