#include <ShadowMapping//Poisson.glsl>

float ShadowPCF(vec4 shadowCoord, uint cascadeIndex)
{
	vec3 N = normalize(normal);
	vec3 L = normalize(shadowUBO.lightPosition.xyz - worldPos);
	float NoL = dot(N, L);

	float bias = max(shadowUBO.shadowMaxBias * (1.0 - NoL), shadowUBO.shadowMinBias);

	// PCF
	/*
	// (1.0 - NoL) allows more blur for vertical surfaces
	float scale = shadowUBO.pcfScale + (1.0 - NoL);
	ivec2 texDim = textureSize(shadowMap, 0).xy;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);
	
	const int range = 1;
	float shadow = 0.0;
	int count = 0;
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			vec2 off = vec2(dx * x, dy * y);
			float dist = texture(shadowMap, vec3(shadowCoord.st + off, cascadeIndex)).r;
			shadow += (shadowCoord.z - bias) > dist ? 0.1 : 1.0;
			count++;
		}
	}
	return shadow / count;*/

	float poissonRadius = 1000.0 * shadowUBO.poissonSize;
	float shadow = 0.0;
	const int numSamples = 8;
	for (int i = 0; i < numSamples; ++i)
	{
		vec2 coord = GetPoissonDiskCoord(shadowCoord.xy, i, poissonRadius);
		float dist = texture(shadowMap, vec3(coord, cascadeIndex)).r;
		shadow += (shadowCoord.z - bias) > dist ? 0.1 : 1.0;
	}
	return shadow / float(numSamples);
}