#version 460 core
#extension GL_EXT_buffer_reference : require

layout(location = 0) in vec2 texCoord;

layout(location = 0) out float fragColor;

#include <SSAO/UBO.glsl>

layout(set = 0, binding = 0) uniform S { SSAOUBO ubo; }; // UBO
layout(set = 0, binding = 1) readonly buffer K { vec3 kernels[]; }; // SSBO
layout(set = 0, binding = 2) uniform sampler2D gPosition;
layout(set = 0, binding = 3) uniform sampler2D gNormal;
layout(set = 0, binding = 4) uniform sampler2D texNoise;

void main()
{
	vec2 noiseScale = vec2(ubo.screenWidth / ubo.noiseSize, ubo.screenHeight / ubo.noiseSize);

	vec4 fragPos = texture(gPosition, texCoord).xyzw;
	vec3 normal = normalize(texture(gNormal, texCoord).rgb);
	vec3 randomVec = normalize(texture(texNoise, texCoord * noiseScale).xyz);

	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
}