#version 460 core

in vec3 b_pos;
in vec3 b_normal;
in vec4 b_posls;
in vec2 b_uv;

uniform vec3 u_diffuse;

uniform mat4 u_lightProj;
uniform float u_shadowBias;

uniform float u_voxelScale;


uniform vec3 u_lightPos;
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform float u_lightInnerCos;
uniform float u_lightOuterCos;


uniform sampler2D u_shadowMap;

layout(RGBA8) uniform image3D u_voxelTexture;

float shadow() {
	vec4 posls = u_lightProj * vec4(b_pos, 1.0);
	float bias = u_shadowBias;
	vec3 coords = posls.xyz / posls.w;
	coords = coords*0.5 + 0.5;
	float shadowDepth = texture(u_shadowMap, coords.xy).r;
	float pointDepth = coords.z;
	float shadowVal = pointDepth >= shadowDepth + bias ? 1.0 : 0.0;
	return shadowVal;
}

vec3 spotlight() {
	vec3 spotDir = u_lightDir;
	vec3 lightDir = u_lightPos - b_pos;
	float dist = abs(length(lightDir));
	lightDir = lightDir / dist;
	float angle = dot(spotDir, -lightDir);
	float intensity = smoothstep(u_lightOuterCos, u_lightInnerCos, angle);
	float attenuation = 1.0 / (1.0 + dist);	
	intensity *= attenuation;
	intensity *= dot(b_normal, lightDir);
	intensity *= 1.0 - shadow();
	return u_lightColor * intensity;
}

bool insideVoxel(vec3 pos) {
  return all(lessThanEqual(pos, vec3(1.0f))) && all(greaterThanEqual(pos, vec3(0.0f)));
}

vec3 toVoxel(vec3 pos) {
  return pos*(0.5/u_voxelScale)+0.5;  
}

void main(){

	vec3 posVoxelSpace = toVoxel(b_pos);

	if (!insideVoxel(posVoxelSpace)) return;

	vec3 color = u_diffuse * spotlight();
	// color = u_diffuse;

	vec3 voxelPos = b_pos/u_voxelScale*0.5 + 0.5;
	ivec3 dim = imageSize(u_voxelTexture);
	vec4 res = vec4(color, 1);
	imageStore(u_voxelTexture, ivec3(dim * posVoxelSpace), res);
}