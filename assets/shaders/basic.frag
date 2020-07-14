#version 460 core

in vec3 a_pos;
in vec2 a_uv;
in vec3 a_normal;
in vec4 a_posls;
// in vec3 a_xtan;
// in vec3 a_ytan;

uniform vec3 u_diffuse;
uniform float u_metal;
uniform float u_rough;
uniform float u_opacity;
uniform vec3 u_emission;

uniform vec3 u_lightPos;
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform float u_lightInnerCos;
uniform float u_lightOuterCos;

uniform float u_shadowBias;

uniform sampler2D u_shadowMap;

layout(location = 0) out vec3 o_albedo;

float shadow() {
	float bias = u_shadowBias;
	vec3 coords = a_posls.xyz / a_posls.w;
	coords = coords*0.5 + 0.5;
	float shadowDepth = texture(u_shadowMap, coords.xy).r;
	float pointDepth = coords.z;
	float shadowVal = pointDepth >= shadowDepth + bias ? 1.0 : 0.0;
	return shadowVal;
}

vec3 spotlight() {
	vec3 spotDir = u_lightDir;
	vec3 lightDir = normalize(u_lightPos - a_pos);
	float dist = abs(length(a_pos - u_lightPos));
	float angle = dot(spotDir, -lightDir);
	float intensity = smoothstep(u_lightOuterCos, u_lightInnerCos, angle);
	float attenuation = 1.0 / (1.0 + dist);	
	intensity *= attenuation;
	intensity *= dot(a_normal, lightDir);
	intensity *= 1.0 - shadow();
	return u_lightColor * intensity;
}

void main() {
  // o_albedo = texture(u_tex, vec3(a_uv, 0)).rgb;
  // o_metal_rough_ao.r = texture(u_tex, vec3(a_uv, 2)).r;
  // o_metal_rough_ao.g = texture(u_tex, vec3(a_uv, 3)).r;
  // o_metal_rough_ao.b = 1.0f;

  // mat3 tangent_matrix = mat3(a_xtan, a_ytan, a_normal);
  // vec3 t_normal = texture(u_tex, vec3(a_uv, 1)).xyz * 2 - 1;
  // o_normal = tangent_matrix * t_normal;

  vec3 lightColor = spotlight();

  vec3 color = lightColor * u_diffuse;
  o_albedo = pow(color, vec3(1.0/2.2));
}