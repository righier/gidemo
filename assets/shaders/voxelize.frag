#version 460 core

in vec3 b_pos;
in vec3 b_posv;
in vec4 b_posls;
in vec3 b_normal;
in vec2 b_uv;

uniform vec3 u_diffuse;

uniform float u_voxelScale;

layout(RGBA8) uniform image3D voxelTexture;


vec3 scaleAndBias(vec3 p) { return 0.5f * p + vec3(0.5f); }

void main(){
	vec3 pos = b_pos * u_voxelScale;

	const vec3 p = abs(b_pos);

	if(max(max(p.x,p.y),p.z) >= 1) return;


	vec3 color = u_diffuse;

	vec3 voxel = scaleAndBias(b_posv);
	ivec3 dim = imageSize(voxelTexture);
	vec4 res = vec4(color, 1);
    imageStore(voxelTexture, ivec3(dim * voxel), res);
}