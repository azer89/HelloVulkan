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

	float occlusion = 0.0;

	for (int i = 0; i < kernels.length(); ++i)
	{
		// Get sample position
		vec3 samplePos = TBN * kernels[i]; // From tangent to view-space
		samplePos = fragPos.xyz + samplePos * ubo.radius;

		// Project sample position (to sample texture) (to get position on screen/texture)
		vec4 offset = vec4(samplePos, 1.0);
		offset = ubo.projection * offset; // From view to clip-space
		offset.xyz /= offset.w; // Perspective divide
		offset.xyz = offset.xyz * 0.5 + 0.5; // Transform to range 0.0 - 1.0

		// Get sample depth
		float sampleDepth = texture(gPosition, offset.xy).z; // Get depth value of kernel sample

		// w = 1.0 if background, 0.0 if foreground
		//float discardFactor = 1.0 - fragPos.w;

		// Range check & accumulate
		//float rangeCheck = smoothstep(0.0, 1.0, ubo.radius / abs(fragPos.z - sampleDepth)) * discardFactor;
		float rangeCheck = smoothstep(0.0, 1.0, ubo.radius / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z + ubo.bias ? 1.0 : 0.0) * rangeCheck;
	}

	occlusion = 1.0 - (occlusion / kernels.length());

	//fragColor = 0.1;
	//fragColor = normal.x;
	fragColor = occlusion;
}