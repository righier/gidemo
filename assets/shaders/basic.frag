
in vec3 a_pos;
in vec2 a_uv;
in vec3 a_normal;
in vec4 a_posls;

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
	intensity *= max(0, dot(a_normal, lightDir));
	intensity *= 1.0 - shadow();
	return u_lightColor * intensity;
}

void main() {

  vec3 lightColor = spotlight();

  vec3 color = lightColor * u_diffuse;
  o_albedo = pow(color, vec3(1.0/2.2));
}