uniform ShadowUBO
{
	mat4 lightSpaceMatrix;
	vec4 lightPosition;
	float shadowMinBias;
	float shadowMaxBias;
	float shadowNearPlane;
	float shadowFarPlane;
	uint pcfIteration;
}
shadowUBO;