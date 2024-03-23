#include <ShadowMapping/Poisson.glsl>

const float SHADOW_AMBIENT = 0.1;
const float PCF_SCALE = 0.5;
const int PCF_RANGE = 1;
const int POISSON_SAMPLE_COUNT = 8;
const float POISSON_RADIUS = 1500.0; // Smaller means blurrier

float ShadowPCF(vec4 shadowCoord)
{
	vec3 N = normalize(normal);
	vec3 L = normalize(shadowUBO.lightPosition.xyz - worldPos);
	float NoL = dot(N, L);

	float bias = max(shadowUBO.shadowMaxBias * (1.0 - NoL), shadowUBO.shadowMinBias);

	// PCF
	// (1.0 - NoL) allows more blur for vertical surfaces
	float scale = PCF_SCALE + (1.0 - NoL);
	ivec2 texDim = textureSize(shadowMap, 0).xy;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);
	
	float shadow = 0.0;
	int count = 0;
	for (int x = -PCF_RANGE; x <= PCF_RANGE; x++)
	{
		for (int y = -PCF_RANGE; y <= PCF_RANGE; y++)
		{
			vec2 off = vec2(dx * x, dy * y);
			float dist = texture(shadowMap, shadowCoord.st + off).r;
			shadow += (shadowCoord.z - bias) > dist ? SHADOW_AMBIENT : 1.0;
			count++;
		}
	}
	return shadow / count;
}

float ShadowPoisson(vec4 shadowCoord)
{
	vec3 N = normalize(normal);
	vec3 L = normalize(shadowUBO.lightPosition.xyz - worldPos);
	float NoL = dot(N, L);

	float bias = max(shadowUBO.shadowMaxBias * (1.0 - NoL), shadowUBO.shadowMinBias);

	float poissonRadius = POISSON_RADIUS;
	float shadow = 0.0;
	
	for (int i = 0; i < POISSON_SAMPLE_COUNT; ++i)
	{
		vec2 coord = GetPoissonDiskCoord(shadowCoord.xy, i, poissonRadius);
		float dist = texture(shadowMap, coord).r;
		shadow += (shadowCoord.z - bias) > dist ? SHADOW_AMBIENT : 1.0;
	}
	return shadow / float(POISSON_SAMPLE_COUNT);
}