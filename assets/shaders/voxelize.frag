#version 460 core

in vec3 b_pos;
in vec3 b_normal;
in vec4 b_posls;
in vec2 b_uv;

uniform vec3 u_diffuse;
uniform vec3 u_emission;

uniform mat4 u_lightProj;
uniform float u_shadowBias;

uniform vec3 u_lightPos;
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform float u_lightInnerCos;
uniform float u_lightOuterCos;

uniform bool u_averageValues;


uniform sampler2D u_shadowMap;

layout(RGBA8) uniform image3D u_voxelTexture;
const ivec3 DIR = ivec3(0, 2, 4);
const int X = 0;
const int Y = 2;
const int Z = 4;

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
  return pos*(0.5)+0.5;  
}

vec4 rgbaToVec(uint val) {
	return vec4(
		float((val&0x000000FF)), 
		float((val&0x0000FF00)>>8U), 
		float((val&0x00FF0000)>>16U), 
		float((val&0xFF000000)>>24U)
	);
}

uint vecToRgba(vec4 val) {
	return (
		(uint(val.w)&0x000000FF)<<24U |
		(uint(val.z)&0x000000FF)<<16U |
		(uint(val.y)&0x000000FF)<<8U  |
		(uint(val.x)&0x000000FF)
	);
}

void imageAtomicAvg(layout(r32ui) coherent volatile image3D imgUI, ivec3 coords, vec4 val) {
	val.rgb *= 255.0f;
	uint newVal = vecToRgba(val);
	uint prevVal = 0;
	uint curVal;

	while ((curVal = imageAtomicCompSwap(imgUI, coords, prevVal, newVal)) != prevVal) {
		prevVal = curVal;
		vec4 rval = rgbaToVec(curVal);
		rval.xyz = (rval.xyz * rval.w);
		vec4 curValF = rval + val;
		curValF.xyz /= curValF.w;
		newVal = vecToRgba(curValF);
	}
}

void main(){

	#if 0
	vec3 albedo = u_diffuse;
	vec3 emission = u_emission;
	#else
	vec3 albedo = pow(u_diffuse, vec3(2.2));
	vec3 emission = pow(u_emission, vec3(2.2));
	#endif

	vec3 posVoxelSpace = toVoxel(b_pos) * 0.999f;
	ivec3 dim = imageSize(u_voxelTexture);

	// if (!insideVoxel(posVoxelSpace)) return;

	vec3 color = albedo * spotlight() + emission;
	// color = u_diffuse;
	// color = b_normal * 0.5 + 0.5;

	vec4 val = vec4(color, 1.0f);

	if (u_averageValues) {
		imageAtomicAvg(u_voxelTexture, ivec3(dim*posVoxelSpace), val);
	} else {
		imageStore(u_voxelTexture, ivec3(dim*posVoxelSpace), val);
	}
}