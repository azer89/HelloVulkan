struct RaytracingUBO
{
	mat4 projInverse;
	mat4 viewInverse;
	vec4 cameraPosition;
	vec4 shadowCasterPosition;
	uint frame;
	uint sampleCountPerFrame;
	uint currentSampleCount;
	uint rayBounceCount;
	float skyIntensity;
	float lightIntensity;
	float specularFuzziness;
};