
/*
Adapted from github.com/SaschaWillems
*/

uint WangHash(inout uint seed)
{
	seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
	seed *= uint(9);
	seed = seed ^ (seed >> 4);
	seed *= uint(0x27d4eb2d);
	seed = seed ^ (seed >> 15);
	return seed;
}

float RandomFloat01(inout uint state)
{
	return float(WangHash(state)) / 4294967296.0;
}

uint NewRandomSeed(uint v0, uint v1, uint v2)
{
	return uint(v0 * uint(1973) + v1 * uint(9277) + v2 * uint(26699)) | uint(1);
}

vec3 RandomInUnitSphere(inout uint seed)
{
	vec3 pos = vec3(0.0);
	do
	{
		pos = vec3(RandomFloat01(seed), RandomFloat01(seed), RandomFloat01(seed)) * 2.0 - 1.0;
	} 
	while (dot(pos, pos) >= 1.0);
	return pos;
}