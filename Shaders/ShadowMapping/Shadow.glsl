#include <ShadowMapping/Poisson.glsl>

const float SHADOW_AMBIENT = 0.1;
const int BLUR_RANGE = 1;
const float PCF_SCALE = 0.5;
const float POISSON_RADIUS = 5000.0; // Smaller means blurrier

// learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
float ShadowPCF(vec4 shadowCoord)
{
	vec3 N = normalize(normal);
	vec3 L = normalize(shadowUBO.lightPosition.xyz - worldPos);
	float NoL = dot(N, L);

	float bias = max(shadowUBO.shadowMaxBias * (1.0 - NoL), shadowUBO.shadowMinBias);

	// PCF
	ivec2 texDim = textureSize(shadowMap, 0).xy;
	float dx = PCF_SCALE / float(texDim.x);
	float dy = PCF_SCALE / float(texDim.y);
	
	float shadow = 0.0;
	int count = 0;
	for (int x = -BLUR_RANGE; x <= BLUR_RANGE; x++)
	{
		for (int y = -BLUR_RANGE; y <= BLUR_RANGE; y++)
		{
			vec2 off = vec2(dx * x, dy * y);
			float dist = texture(shadowMap, shadowCoord.st + off).r;
			shadow += (shadowCoord.z - bias) > dist ? SHADOW_AMBIENT : 1.0;
			count++;
		}
	}
	return shadow / count;
}

// This is a combination of
// github.com/opengl-tutorials/ogl/blob/master/tutorial16_shadowmaps/ShadowMapping.fragmentshader
// and
// learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
float ShadowPoisson(vec4 shadowCoord)
{
	vec3 N = normalize(normal);
	vec3 L = normalize(shadowUBO.lightPosition.xyz - worldPos);
	float NoL = dot(N, L);

	float bias = max(shadowUBO.shadowMaxBias * (1.0 - NoL), shadowUBO.shadowMinBias);

	ivec2 texDim = textureSize(shadowMap, 0).xy;
	float dx = 1.0 / float(texDim.x);
	float dy = 1.0 / float(texDim.y);

	float shadow = 0.0;
	int count = 0;
	for (int x = -BLUR_RANGE; x <= BLUR_RANGE; x++)
	{
		for (int y = -BLUR_RANGE; y <= BLUR_RANGE; y++)
		{
			vec2 off = vec2(dx * x, dy * y);
			vec2 coord = GetPoissonDiskCoord(shadowCoord.st + off, count, POISSON_RADIUS);
			float dist = texture(shadowMap, coord).r;
			shadow += (shadowCoord.z - bias) > dist ? SHADOW_AMBIENT : 1.0;
			count++;
		}
	}
	return shadow / float(count);
}