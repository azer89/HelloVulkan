struct RayPayload
{
	vec3 color;
	vec3 scatterDir;
	float distance;
	uint randomSeed;
	bool doScatter;
};