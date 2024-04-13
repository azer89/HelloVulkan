#version 460 core
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

#include <CameraUBO.glsl>
#include <Bindless/VertexData.glsl>
#include <Bindless/MeshData.glsl>
#include <Bindless/BDA.glsl>

layout(location = 0) in vec3 viewPos;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texCoord;
layout(location = 4) in flat uint meshIndex;

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec3 gNormal;

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; }; // UBO
layout(set = 0, binding = 2) uniform BDABlock { BDA bda; }; // UBO
layout(set = 0, binding = 3) uniform sampler2D pbrTextures[] ;

float LinearDepth(float z, float near, float far)
{
	return near * far / (far + z * (near - far));
}

void main()
{
	MeshData mData = bda.meshReference.meshes[meshIndex];

	// TODO This kills performance
	float alpha = texture(pbrTextures[nonuniformEXT(mData.albedo)], texCoord).a;
	if (alpha < 0.5)
	{
		discard;
	}

	// TODO gl_FragCoord.z is always 1.0 for some reason
	gPosition = vec4(viewPos, LinearDepth(fragPos.z, camUBO.cameraNear, camUBO.cameraFar));
	gNormal = normalize(normal) * 0.5 + 0.5;
}