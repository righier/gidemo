#version 460 core

layout (location = 0) in vec3 i_position;
// layout (location = 1) in vec2 i_uv;
// layout (location = 2) in vec3 i_normal;
// layout (location = 3) in vec3 i_xtan;
// layout (location = 4) in vec3 i_ytan;

out vec3 a_pos;

uniform mat4 u_project;

void main() {
	vec4 pos = vec4(i_position, 1.0f);
	a_pos = pos.xyz;
	gl_Position = u_project * pos;
}