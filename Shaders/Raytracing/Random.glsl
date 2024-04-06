
/*
Adapted from github.com/SaschaWillems
*/

const float PI = 3.14159265359;
const float PI_2 = 2.0 * PI;

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

// Tiny Encryption Algorithm
// By Fahad Zafar, Marc Olano and Aaron Curtis
// www.highperformancegraphics.org/previous/www_2010/media/GPUAlgorithms/HPG2010_GPUAlgorithms_Zafar.pdf
uint TEA(uint val0, uint val1)
{
	uint sum = 0;
	uint v0 = val0;
	uint v1 = val1;
	for (uint n = 0; n < 16; n++)
	{
		sum += 0x9E3779B9;
		v0 += ((v1 << 4) + 0xA341316C) ^ (v1 + sum) ^ ((v1 >> 5) + 0xC8013EA4);
		v1 += ((v0 << 4) + 0xAD90777D) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7E95761E);
	}
	return v0;
}

// Linear congruential generator based on the previous RNG state
// See https://en.wikipedia.org/wiki/Linear_congruential_generator
uint LCG(inout uint previous)
{
	const uint multiplier = 1664525u;
	const uint increment = 1013904223u;
	previous = (multiplier * previous + increment);
	return previous & 0x00FFFFFF;
}

// Generate a random float in [0, 1) given the previous RNG state
float RND(inout uint previous)
{
	return (float(LCG(previous)) / float(0x01000000));
}