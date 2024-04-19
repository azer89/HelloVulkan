
#extension GL_EXT_control_flow_attributes : require

// Generates a seed for a random number generator from 2 inputs plus a backoff
// github.com/nvpro-samples/optix_prime_baking/blob/332a886f1ac46c0b3eea9e89a59593470c755a0e/random.h
// github.com/nvpro-samples/vk_raytracing_tutorial_KHR/tree/master/ray_tracing_jitter_cam
// en.wikipedia.org/wiki/Tiny_Encryption_Algorithm
uint InitRandomSeed(uint val0, uint val1)
{
    uint v0 = val0, v1 = val1, s0 = 0;

    [[unroll]]
    for (uint n = 0; n < 16; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }

    return v0;
}

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