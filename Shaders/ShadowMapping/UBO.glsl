#define SHADOW_MAP_CASCADE_COUNT 4

struct ShadowUBO
{
	mat4 lightSpaceMatrices[SHADOW_MAP_CASCADE_COUNT];
	vec4 splitDepths;
	vec4 lightPosition;
	float shadowMinBias;
	float shadowMaxBias;
	float cameraNear;
	float cameraFar;
	float poissonSize;
};