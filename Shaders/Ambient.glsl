vec3 Ambient(
	vec3 albedo,
	vec3 F0,
	vec3 N,
	vec3 V,
	float metallic,
	float roughness,
	float ao,
	float NoV)
{
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

	return (kD * diffuse + specular) * ao;
}
