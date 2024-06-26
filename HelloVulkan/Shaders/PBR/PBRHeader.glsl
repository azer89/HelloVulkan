
/*
Notations
	V	View unit vector
	L	Incident light unit vector
	N	Surface normal unit vector
	H	Half unit vector between L and V
*/

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

// Specular D
// Trowbridge-Reitz GGX that models the distribution of microfacet normal.
float DistributionGGX(float NoH, float roughness)
{
	float alpha = roughness * roughness; // Disney remapping
	float alpha2 = alpha * alpha;
	float NoH2 = NoH * NoH;

	float nominator = alpha2;
	float denominator = (NoH2 * (alpha2 - 1.0) + 1.0);
	denominator = PI * denominator * denominator;

	return nominator / denominator;
}

// Roughness remapping for direct lighting (See Brian Karis's PBR Note)
float AlphaDirectLighting(float roughness)
{
	float r = (roughness + 1.0);
	float alpha = (r * r) / 8.0;
	return alpha;
}

// Specular G
// Geometry function that describes the self-shadowing property of the microfacets.
// When a surface is relatively rough, the surface's microfacets can overshadow other
// microfacets reducing the light the surface reflects.
float GeometrySchlickGGX(float NoL, float NoV, float alpha)
{
	float GL = NoL / (NoL * (1.0 - alpha) + alpha);
	float GV = NoV / (NoV * (1.0 - alpha) + alpha);
	return GL * GV;
}

// Specular F
// The Fresnel equation describes the ratio of surface reflection at different surface angles.
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Basic Lambertian diffuse
vec3 Diffuse(vec3 albedo)
{
	return albedo / PI;
}