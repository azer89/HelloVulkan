const int NUM_ELEMENTS_DISK = 16;

const vec2 poissonDisk[NUM_ELEMENTS_DISK] = vec2[](
	vec2(-0.5119625f, -0.4827938f),
	vec2(-0.2171264f, -0.4768726f),
	vec2(-0.7552931f, -0.2426507f),
	vec2(-0.7136765f, -0.4496614f),
	vec2(-0.5938849f, -0.6895654f),
	vec2(-0.3148003f, -0.7047654f),
	vec2(-0.42215f, -0.2024607f),
	vec2(-0.9466816f, -0.2014508f),
	vec2(-0.8409063f, -0.03465778f),
	vec2(-0.6517572f, -0.07476326f),
	vec2(-0.1041822f, -0.02521214f),
	vec2(-0.3042712f, -0.02195431f),
	vec2(-0.5082307f, 0.1079806f),
	vec2(-0.08429877f, -0.2316298f),
	vec2(-0.9879128f, 0.1113683f),
	vec2(-0.3859636f, 0.3363545f));

// Returns a pseudo random number based on a vec3 and an int.
float PseudoRandom(vec3 seed, int i)
{
	vec4 seed4 = vec4(seed, i);
	float dotValue = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
	return fract(sin(dotValue) * 43758.5453);
}

vec2 GetPoissonDiskCoord(vec2 projCoords, int i, float radius)
{
	int index = int(float(NUM_ELEMENTS_DISK) * PseudoRandom(projCoords.xyy, i)) % NUM_ELEMENTS_DISK;
	return projCoords.xy + poissonDisk[index] / radius;
}