layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_uv;

uniform mat4 u_project;
uniform mat4 u_transform;

out vec3 a_pos;
out vec2 a_uv;

void main() {
	vec4 pos = u_transform * vec4(i_position, 1.0f);
	gl_Position = u_project * pos;
	a_pos = pos.xyz;
	a_uv = i_uv;
}