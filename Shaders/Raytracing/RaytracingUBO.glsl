struct RaytracingUBO
{
	mat4 projInverse;
	mat4 viewInverse;
	vec4 cameraPosition;
	uint frame; // Used for blending/Anti aliasing
	uint sampleCountPerFrame;
	uint currentSampleCount;
	uint rayBounceCount;
};