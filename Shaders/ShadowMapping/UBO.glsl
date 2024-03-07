struct ShadowUBO
{
	mat4 lightSpaceMatrix;
	vec4 lightPosition;
	float shadowMinBias;
	float shadowMaxBias;
};