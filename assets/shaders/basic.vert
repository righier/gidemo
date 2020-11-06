
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_uv;
layout (location = 2) in vec3 i_normal;

out vec3 a_pos;
out vec3 a_normal;
out vec4 a_posls;
out vec2 a_uv;

uniform mat4 u_project;
uniform mat4 u_transform;
uniform mat4 u_rotate;
uniform mat4 u_lightProj;

void main() {
	vec4 pos = u_transform * vec4(i_position, 1.0f);
	a_pos = pos.xyz;
	gl_Position = u_project * pos;
	a_posls = u_lightProj * pos;

	mat3 rotate = mat3(transpose(inverse(u_transform)));

	a_normal = normalize(rotate * i_normal);
}