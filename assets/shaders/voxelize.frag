#version 460 core

in vec3 b_pos;
in vec3 b_normal;
in vec4 b_posls;
in vec2 b_uv;

uniform vec3 u_diffuse;
uniform vec3 u_emission;
uniform float u_metal;
uniform float u_rough;

uniform mat4 u_lightProj;
uniform float u_shadowBias;

uniform vec3 u_lightPos;
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform float u_lightInnerCos;
uniform float u_lightOuterCos;

uniform bool u_averageValues;

uniform int u_voxelCount;
uniform bool u_aniso;
uniform bool u_temporalMultibounce;

uniform float u_time;
float time = mod(u_time, 1.0f);

uniform sampler2D u_shadowMap;

layout(RGBA8) uniform image3D u_voxelTexture;

uniform sampler3D u_voxelTextureAniso[6];
uniform sampler3D u_voxelTexturePrev;

float voxelSize = 1.0f / float(u_voxelCount);

const float PI = 3.14159265359;

vec4 fetch(vec3 dir, vec3 pos, float lod) {
	pos = clamp(pos, vec3(0), vec3(1));
	if (u_aniso) {
		bvec3 sig = greaterThan(dir, vec3(0.0f));
		float l2 = lod;
		vec4 cx = sig.x ? textureLod(u_voxelTextureAniso[1], pos, l2) : textureLod(u_voxelTextureAniso[0], pos, l2);
		vec4 cy = sig.y ? textureLod(u_voxelTextureAniso[3], pos, l2) : textureLod(u_voxelTextureAniso[2], pos, l2);
		vec4 cz = sig.z ? textureLod(u_voxelTextureAniso[5], pos, l2) : textureLod(u_voxelTextureAniso[4], pos, l2);

		vec3 sd = dir * dir; 
		vec4 c1 = sd.x * cx + sd.y * cy + sd.z * cz;
		return c1;
	} else {
		return textureLod(u_voxelTexturePrev, pos, max(1, lod));
	}
}

float shadow() {
	vec4 coords = u_lightProj * vec4(b_pos, 1.0);
	float bias = u_shadowBias;
	coords = coords*0.5;
	coords += coords.w;
	float total = 0;
	const int nsamples = 3;
	float scale = voxelSize / 2;
	// scale = 03
	for (int i = 0; i < nsamples; i++) {
		for (int j = 0; j < nsamples; j++) {
			vec2 offset = vec2(i, j) / nsamples * 2 - 1;
			offset *= scale;
			vec4 pos = coords + vec4(offset, 0, 0);

			float shadowDepth = textureProj(u_shadowMap, pos.xyw).r;
			float pointDepth = coords.z / coords.w;
			float val = float(pointDepth - bias >= shadowDepth);
			total += val;
		}
	}

	total /= nsamples*nsamples;

	// float shadowDepth = textureProj(u_shadowMap, coords.xyw).r;
	// float pointDepth = (coords.z) / coords.w;
	// float shadowVal = float(pointDepth - bias >= shadowDepth);
	return total;
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

vec3 ortho(vec3 v) {
  vec3 w = abs(dot(v, vec3(0,0,1))) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
  return cross(v, w);
}

bool insideVoxel(vec3 pos) {
  return all(lessThanEqual(pos, vec3(1.0f))) && all(greaterThanEqual(pos, vec3(0.0f)));
}

vec3 toVoxel(vec3 pos) {
  return pos*(0.5)+0.5;  
}

float random(vec3 v) {
  float a = dot(v, vec3(29.29384, 62.29384, 48.23478));
  a = sin(a);
  a = a * 293487.1273498;
  a = fract(a);
  return a;
}

vec3 random3(vec3 p3){
  p3 = fract(p3 * vec3(443.897, 441.423, 437.195));
  p3 += dot(p3, p3.zxy + vec3(31.31));
  return fract((p3.xyz + p3.yzx) * p3.zyx);
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
	vec4 v = clamp(val, vec4(0), vec4(255));
	// vec4 v = val;
	return (
		(uint(v.w)&0x000000FF)<<24U |
		(uint(v.z)&0x000000FF)<<16U |
		(uint(v.y)&0x000000FF)<<8U  |
		(uint(v.x)&0x000000FF)
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

vec3 myTracing(vec3 from, vec3 dir, float apertureTan2) {
  vec4 color = vec4(0);
  float dist = voxelSize * 0;

  // float a2 = max(voxelSize, 2 * tan(apertureAngle / 2));
  float a2 = max(voxelSize, apertureTan2);

  while (dist < 1.733 && color.a < 1.0) {
    float diameter = max(voxelSize, dist * a2);
    float lod = log2(diameter * u_voxelCount);//* u_voxelCount);
    vec3 pos = from + dir * dist;
    // color = vec4(color.rgb, 1) * color.a + (1 - color.a) * fetch(dir, pos, lod);
    color += (1 - color.a) * fetch(dir, pos, lod);

    dist += diameter * 1.0;
  }
  // return vec3(1);

  return color.rgb;
}

vec3 myDiffuse() {
  vec3 color = vec3(0);

  vec3 pos = toVoxel(b_pos);
  pos = (floor(pos * 0.999 * u_voxelCount) + 0.5) * voxelSize;
  vec3 normal = b_normal;
  pos += 8 * voxelSize * normal;

  #define CONES 5

  const vec3 cones[5] = vec3[5](
    vec3(0, 0, 1),
    vec3(1, 0, 1),
    vec3(-1, 0, 1),
    vec3(0, 1, 1),
    vec3(0, -1, 1)
    );

  const float weight[5] = float[5]( 1, 0.2, 0.2, 0.2, 0.2);

  float apertureAngle = 30;
  float apertureTan2 = 2 * tan(radians(apertureAngle)/2);


  vec3 xtan, ytan;
 
  // xtan = ortho(normal);
  xtan = random3(pos * time);
  ytan = cross(normal, xtan);
  xtan = cross(normal, ytan);
  // ytan = cross(xtan, normal);

  mat3 tanSpace = mat3(xtan, ytan, normal); 

  for (int i = 0; i < CONES; i++) {
    vec3 dir = tanSpace * cones[i];
    color += weight[i] * myTracing(pos, normalize(dir), apertureTan2);
  }


  // return vec3(1);

  return color;
}


//
float NormalDistributionTrowbridgeReitz(float NdotH, float roughness) {
  float a = roughness*roughness;
  float a2 = a*a;
  float NdotH2 = NdotH*NdotH;
  float d = NdotH2 * (a2 - 1) + 1;
  return a2 / (PI * d*d);
}

float NormalDistributionGGX(float NdotH, float roughness) {
  float a = roughness*roughness;
  float NdotH2 = NdotH*NdotH;
  float TanNdotH2 = (1.0 - NdotH2) / NdotH2;
  float d = roughness / (NdotH2 * (a + TanNdotH2));
  return (1.0 / PI) * d*d;
}

// Geometrix Shadowing (Smith's method, Schlick Approximation)
// remaps roughness to (r + 1) / 2
float GeometrySchlickFixed(float NdotL, float NdotV, float roughness) {
  float r = (roughness + 1);
  float k = (r*r) / 8;
  vec2 gs = k + (1 - k) * vec2(NdotL, NdotV);
  return (NdotL * NdotV) / (gs.x * gs.y);
}

float GeometrySchlick(float NdotL, float NdotV, float roughness) {
  float k = (roughness*roughness) / 2;
  vec2 gs = k + (1 - k) * vec2(NdotL, NdotV);
  return (NdotL * NdotV) / (gs.x * gs.y);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1 - F0) * pow(1 - cosTheta, 5);
}

vec3 FresnelGaussian(float cosTheta, vec3 F0) {
  return F0 + (1.0f - F0) * exp2(cosTheta * (-5.55473*cosTheta - 6.98316));
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
} 

void main(){

	#if 0
	vec3 albedo = u_diffuse;
	vec3 emission = u_emission;
	#else
	vec3 albedo = pow(u_diffuse, vec3(2.2));
	vec3 emission = pow(u_emission, vec3(2.2));
	#endif

	float rougness = u_rough;
	float metalness = u_metal;
	vec3 normal = b_normal;
	vec3 pos = b_pos;


	vec3 color = albedo * spotlight() / PI;

	// color = u_diffuse;
	// color = b_normal * 0.5 + 0.5;

	if (u_temporalMultibounce) {
		vec3 indirect = myDiffuse();
		indirect *= albedo;
		color += indirect * 0.3;
	}
	color += emission;

	vec4 val = vec4(color, 1.0f);

	vec3 posVoxelSpace = toVoxel(b_pos) * 0.999f;
	ivec3 dim = imageSize(u_voxelTexture);
	if (u_averageValues) {
		imageAtomicAvg(u_voxelTexture, ivec3(dim*posVoxelSpace), val);
	} else {
		imageStore(u_voxelTexture, ivec3(dim*posVoxelSpace), val);
	}
}