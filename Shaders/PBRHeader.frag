
#define PI 3.1415926535897932384626433832795

// Normal distribution function - Trowbridge-Reitz GGX
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NoH = max(dot(N, H), 0.0);
	float NoH2 = NoH * NoH;

	float nominator = a2;
	float denominator = (NoH2 * (a2 - 1.0) + 1.0);
	denominator = PI * denominator * denominator;

	return nominator / denominator;
}

float GeometrySchlickGGX_IBL(float dotNL, float dotNV, float roughness)
{
	// k_IBL is roughness remapping for IBL lighting
	float k_IBL = (roughness * roughness) / 2.0;
	float GL = dotNL / (dotNL * (1.0 - k_IBL) + k_IBL);
	float GV = dotNV / (dotNV * (1.0 - k_IBL) + k_IBL);
	return GL * GV;
}

float GeometrySchlickGGX_Direct(float dotNL, float dotNV, float roughness)
{
	// k_direct is roughness remapping for direct lighting
	float r = (roughness + 1.0);
	float k_direct = (r * r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k_direct) + k_direct);
	float GV = dotNV / (dotNV * (1.0 - k_direct) + k_direct);
	return GL * GV;
}

// The ratio of surface reflection at different surface angles
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// From spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// From tangent-space H vector to world-space sample vector
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}