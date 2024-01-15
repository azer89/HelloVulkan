#version 460 core

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform PerFrameUBO
{
	mat4 cameraProjection;
	mat4 cameraView;
	vec4 cameraPosition;
}
frameUBO;

layout(push_constant) uniform PushConstantPBR
{
	float lightIntensity;
	float baseReflectivity;
	float maxReflectionLod;
}
pc;

// SSBO
struct LightData
{
	vec4 position;
	vec4 color;
};
layout(binding = 2) readonly buffer Lights { LightData data []; } inLights;

layout(binding = 3) uniform sampler2D textureAlbedo;
layout(binding = 4) uniform sampler2D textureNormal;
layout(binding = 5) uniform sampler2D textureMetalness;
layout(binding = 6) uniform sampler2D textureRoughness;
layout(binding = 7) uniform sampler2D textureAO;
layout(binding = 8) uniform sampler2D textureEmissive;

layout(binding = 9) uniform samplerCube specularMap;
layout(binding = 10) uniform samplerCube diffuseMap;
layout(binding = 11) uniform sampler2D brdfLUT;

// Include files
#include <PBRHeader.frag>
#include <Hammersley.frag>

// Tangent-normals to world-space
vec3 GetNormalFromMap(vec3 tangentNormal, vec3 worldPos, vec3 normal, vec2 texCoord)
{
	vec3 Q1 = dFdx(worldPos);
	vec3 Q2 = dFdy(worldPos);
	vec2 st1 = dFdx(texCoord);
	vec2 st2 = dFdy(texCoord);

	vec3 N = normalize(normal);
	vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

void main()
{
	vec4 albedo4 = texture(textureAlbedo, texCoord).rgba;

	if (albedo4.a < 0.5)
	{
		discard;
	}

	// Material properties
	vec3 albedo = pow(albedo4.rgb, vec3(2.2)); 
	vec3 emissive = texture(textureEmissive, texCoord).rgb;
	float metallic = texture(textureMetalness, texCoord).b;
	float roughness = texture(textureRoughness, texCoord).g;
	float alpha = AlphaDirectLighting(roughness);
	float ao = texture(textureAO, texCoord).r;

	vec3 tangentNormal = texture(textureNormal, texCoord).xyz * 2.0 - 1.0;

	// Input lighting data
	vec3 N = GetNormalFromMap(tangentNormal, worldPos, normal, texCoord);
	vec3 V = normalize(frameUBO.cameraPosition.xyz - worldPos);
	float NoV = max(dot(N, V), 0.0);

	// Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)  
	vec3 F0 = vec3(pc.baseReflectivity);
	F0 = mix(F0, albedo, metallic);

	// Reflectance equation
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < inLights.data.length(); ++i)
	{
		LightData light = inLights.data[i];
		light.color *= pc.lightIntensity;

		vec3 L = normalize(light.position.xyz - worldPos); // Incident light vector
		vec3 H = normalize(V + L); // Halfway vector
		float NoH = max(dot(N, H), 0.0);
		float NoL = max(dot(N, L), 0.0);
		float HoV = max(dot(H, V), 0.0);
		float distance = length(light.position.xyz - worldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = light.color.xyz * attenuation;
		
		// Cook-Torrance BRDF
		float D = DistributionGGX(NoH, roughness);
		float G = GeometrySchlickGGX(NoL, NoV, alpha);
		vec3 F = FresnelSchlick(HoV, F0);

		vec3 numerator = D * G * F;
		float denominator = 4.0 * NoV * NoL + 0.0001; // + 0.0001 to prevent divide by zero
		vec3 specular = numerator / denominator;

		// kS is equal to Fresnel
		vec3 kS = F;
		// For energy conservation, the diffuse and specular light can't
		// be above 1.0 (unless the surface emits light); to preserve this
		// relationship the diffuse component (kD) should equal 1.0 - kS.
		vec3 kD = vec3(1.0) - kS;
		// Multiply kD by the inverse metalness such that only non-metals 
		// have diffuse lighting, or a linear blend if partly metal (pure metals
		// have no diffuse light).
		kD *= 1.0 - metallic;

		vec3 diffuse = Diffuse(kD * albedo);

		// Add to outgoing radiance Lo
		Lo += (diffuse + specular) * radiance * NoL; // Note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
	}

	// Ambient lighting (we now use IBL as the ambient term)
	vec3 F = FresnelSchlickRoughness(NoV, F0, roughness);

	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	vec3 irradiance = texture(diffuseMap, N).rgb;
	vec3 diffuse = irradiance * albedo;

	// Sample both the pre-filter map and the BRDF lut and combine them together as
	// per the Split-Sum approximation to get the IBL specular part.
	vec3 R = reflect(-V, N);
	vec3 prefilteredColor = textureLod(specularMap, R, roughness * pc.maxReflectionLod).rgb;
	vec2 brdf = texture(brdfLUT, vec2(NoV, roughness)).rg;
	vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular) * ao;
	vec3 color = ambient + emissive + Lo;

	fragColor = vec4(color, 1.0);
}