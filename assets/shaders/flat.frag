#version 460 core

in vec3 a_pos;
in vec2 a_uv;
in vec3 a_normal;
// in vec3 a_xtan;
// in vec3 a_ytan;

uniform vec3 u_diffuse;
// uniform float u_metal;
// uniform float u_rough;
// uniform float u_opacity;
// uniform vec3 u_emission;

layout(location = 0) out vec3 o_albedo;

void main() {

	// mat3 tangent_matrix = mat3(a_xtan, a_ytan, a_normal);
	// vec3 t_normal = texture(u_tex, a_uv).xyz * 2 - 1;
	// vec3 o_normal = tangent_matrix * t_normal;
	//vec3 o_normal = a_normal;

	// vec3 light_pos = vec3(-1.0f, 5.0f, 1.0f);

	//o_albedo = vec3(0.0f, 1.0f, 0.0f);
	//o_albedo = vec3(texture(u_tex, a_uv).xyz);

	// o_albedo = o_normal;

	// vec3 light_dir = normalize(light_pos - a_pos);
	// light_dir = normalize(vec3(1,1,1));
	// float lum = max(0, dot(light_dir, o_normal));
	// o_albedo = vec3(lum);

	o_albedo = a_normal * 0.5 + 0.5;
	// o_albedo = u_diffuse;
}

