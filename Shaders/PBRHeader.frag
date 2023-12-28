
/*
Notations
	V	View unit vector
	L	Incident light unit vector
	N	Surface normal unit vector
	H	Half unit vector between L and V
*/

#define PI 3.1415926535897932384626433832795

// Normal distribution function - Trowbridge-Reitz GGX
float DistributionGGX(float NoH, float roughness)
{
	float a = roughness * roughness; // Reparameterization
	float a2 = a * a;
	float NoH2 = NoH * NoH;

	float nominator = a2;
	float denominator = (NoH2 * (a2 - 1.0) + 1.0);
	denominator = PI * denominator * denominator;

	return nominator / denominator;
}

float GeometrySchlickGGX_IBL(float NoL, float NoV, float roughness)
{
	// k_IBL is roughness remapping for IBL lighting
	float k_IBL = (roughness * roughness) / 2.0;
	float GL = NoL / (NoL * (1.0 - k_IBL) + k_IBL);
	float GV = NoV / (NoV * (1.0 - k_IBL) + k_IBL);
	return GL * GV;
}

float GeometrySchlickGGX_Direct(float NoL, float NoV, float roughness)
{
	// k_direct is roughness remapping for direct lighting (See Brian Karis's PBR Note)
	float r = (roughness + 1.0);
	float k_direct = (r * r) / 8.0;
	float GL = NoL / (NoL * (1.0 - k_direct) + k_direct);
	float GV = NoV / (NoV * (1.0 - k_direct) + k_direct);
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