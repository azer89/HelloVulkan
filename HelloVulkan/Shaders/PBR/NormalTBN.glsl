
vec3 NormalTBN(vec3 textureNormal, vec3 worldPos, vec3 normal, vec2 texCoord)
{
	vec3 Q1 = dFdx(worldPos);
	vec3 Q2 = dFdy(worldPos);
	vec2 st1 = dFdx(texCoord);
	vec2 st2 = dFdy(texCoord);

	vec3 N = normalize(normal);
	vec3 T = normalize(Q1 * st2.y - Q2 * st1.y);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * textureNormal);
}