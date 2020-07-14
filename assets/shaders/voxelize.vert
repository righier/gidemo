#version 460 core

layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_uv;
layout (location = 2) in vec3 i_normal;
layout (location = 3) in vec3 i_xtan;
layout (location = 4) in vec3 i_ytan;

uniform mat4 u_transform;

uniform float u_voxelScale;

out vec3 a_pos;
out vec3 a_normal;
out vec2 a_uv;
// out vec3 o_xtan;
// out vec3 o_ytan;

void main() {
	vec4 pos = u_transform * vec4(i_position, 1.0f);
	a_pos = pos.xyz;
	gl_Position = vec4(pos.xyz / u_voxelScale, 1.0f);

	mat3 rotate = mat3(transpose(inverse(u_transform)));

	a_normal = normalize(rotate * i_normal);
	// o_xtan = rotate * i_xtan;
	// o_ytan = rotate * i_ytan;

	a_uv = i_uv;
}