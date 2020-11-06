
in vec3 b_pos;
in vec3 b_normal;
in vec3 b_xtan;
in vec3 b_ytan;
in vec4 b_posls;
in vec2 b_uv;

uniform vec3 u_diffuse;
uniform vec3 u_emission;
uniform float u_emissionScale;
uniform float u_metal;
uniform float u_rough;

uniform bool u_particle;
uniform float u_opacity;

uniform bool u_useMaps;
uniform sampler2D u_diffuseMap;
uniform sampler2D u_bumpMap;
uniform sampler2D u_emissionMap;

uniform mat4 u_lightProj;
uniform float u_shadowBias;

uniform vec3 u_lightPos;
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform float u_lightInnerCos;
uniform float u_lightOuterCos;

uniform bool u_averageValues;
uniform float u_restitution;

uniform int u_voxelCount;
uniform bool u_temporalMultibounce;

uniform float u_time;
float time = mod(u_time, 1.0f);

uniform sampler2D u_shadowMap;

layout(RGBA8) uniform image3D u_voxelTexture;

#ifdef ANISO
uniform sampler3D u_voxelTexturePrev[6];
#else
uniform sampler3D u_voxelTexturePrev;
#endif

float voxelSize = 1.0f / float(u_voxelCount);

const float PI = 3.14159265359;

vec4 fetch(vec3 dir, vec3 pos, float lod) {
#ifdef ANISO
	bvec3 sig = greaterThan(dir, vec3(0.0f));
	float l2 = lod;

	vec4 cx = sig.x ? textureLod(u_voxelTexturePrev[1], pos, l2) : textureLod(u_voxelTexturePrev[0], pos, l2);
	vec4 cy = sig.y ? textureLod(u_voxelTexturePrev[3], pos, l2) : textureLod(u_voxelTexturePrev[2], pos, l2);
	vec4 cz = sig.z ? textureLod(u_voxelTexturePrev[5], pos, l2) : textureLod(u_voxelTexturePrev[4], pos, l2);

	vec3 sd = dir * dir; 
	vec4 c1 = sd.x * cx + sd.y * cy + sd.z * cz;
	return c1;
#else
	return textureLod(u_voxelTexturePrev, pos, max(1, lod));
#endif
}

float shadow() {
	vec4 coords = u_lightProj * vec4(b_pos, 1.0);
	float bias = u_shadowBias;
	coords = coords*0.5;
	coords += coords.w;
	float total = 0;
	const int nsamples = 3;
	float scale = voxelSize/2;
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

	return clamp(total, 0, 1);
	return total;
}

vec3 spotlight(vec3 normal) {
	vec3 spotDir = u_lightDir;
	vec3 lightDir = u_lightPos - b_pos;
	float dist = abs(length(lightDir));
	lightDir = lightDir / dist;
	float angle = dot(spotDir, -lightDir);
	float intensity = smoothstep(u_lightOuterCos, u_lightInnerCos, angle);
	float attenuation = 1.0 / (1.0 + dist);	
	intensity *= attenuation;
	intensity *= max(0, dot(normal, lightDir));
	if (intensity>0) intensity *= 1.0 - shadow();
	return u_lightColor * intensity;
}

vec3 ortho(vec3 v) {
  vec3 w = abs(dot(v, normalize(vec3(.1,0,1)))) < 0.999 ? vec3(.1,0,1) : vec3(1,0,0);
  return normalize(cross(v, w));
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

vec3 ConeTracing(vec3 from, vec3 dir, float apertureTan2) {
  vec4 color = vec4(0);
  float dist = voxelSize * 0;

  float a2 = max(voxelSize, apertureTan2);

  while (dist < 1.733 && color.a < 1.0) {
    float diameter = max(voxelSize, dist * a2);
    float lod = log2(diameter * u_voxelCount);
    vec3 pos = from + dir * dist;
    if (!insideVoxel(pos)) break;
    vec4 cc = fetch(dir, pos, lod);
#ifndef ANISO
    cc = cc * pow(2, lod);
#endif
    color += (1 - color.a) * cc;

    dist += diameter * 0.7;
  }

  return color.rgb;
}

vec3 TraceDiffuse(vec3 normal) {
  vec3 color = vec3(0);

  vec3 pos = toVoxel(b_pos);
  pos += 8 * voxelSize * normal;

  #define CONES 5

  const vec3 cones[9] = vec3[9]( 
    vec3(0,0,1), 
    vec3(1,0,1), 
    vec3(-1,0,1), 
    vec3(0,1,1), 
    vec3(0,-1,1),
    vec3(1,1,1),
    vec3(1,-1,1),
    vec3(-1,1,1),
    vec3(-1,-1,1)
    );

  float apertureAngle = 30;
  float apertureTan2 = 2 * tan(radians(apertureAngle)/2);

  vec3 xtan, ytan;
 
  xtan = ortho(normal);
  ytan = cross(normal, xtan);
  xtan = cross(normal, ytan);

  mat3 tanSpace = mat3(xtan, ytan, normal); 

  for (int i = 0; i < CONES; i++) {
    vec3 dir = tanSpace * cones[i];
    color += ConeTracing(pos, normalize(dir), apertureTan2);
  }

  return color;
}

void main(){

	vec3 posVoxelSpace = toVoxel(b_pos) * 0.999f;
	ivec3 dim = imageSize(u_voxelTexture);

	float gammaEditor = 2.2;
	float gammaTextures = 2.2;

	if (u_particle) {
		vec3 pos = b_pos;
		vec3 emission = pow(u_emission, vec3(gammaEditor));
		float opacity = texture(u_emissionMap, b_uv).r * u_opacity;

		vec4 color = vec4(emission * opacity, 0.0);

		vec4 prev = imageLoad(u_voxelTexture, ivec3(dim*posVoxelSpace));
		color += prev;
		imageStore(u_voxelTexture, ivec3(dim*posVoxelSpace), color);
		return;
	}

	vec3 pos = b_pos;
	vec3 normal = b_normal;

	vec3 albedo = pow(u_diffuse, vec3(gammaEditor));
	vec3 emission = pow(u_emission, vec3(gammaEditor));

	float rougness = u_rough;
	float metalness = u_metal;

	if (u_useMaps) {
		albedo *= pow(texture(u_diffuseMap, b_uv).rgb, vec3(gammaTextures));
		emission *= pow(texture(u_emissionMap, b_uv).rgb, vec3(gammaTextures));

	    mat3 tanSpace = mat3(normalize(b_xtan), normalize(b_ytan), normalize(b_normal));
	    vec2 tn = texture(u_bumpMap, b_uv).rg * 2 - 1;
	    normal = vec3(tn, sqrt(1 - dot(tn, tn)));

	    normal = tanSpace * normal;	
	}

	emission *= u_emissionScale;

	vec3 color = albedo * spotlight(normal) / PI;

	if (u_temporalMultibounce) {
		vec3 indirect = TraceDiffuse(normal);
		indirect *= albedo;
		color += indirect * u_restitution * 0.8;
	}
	color += emission;

	vec4 val = vec4(color, 1.0f);

	if (u_averageValues) {
		imageAtomicAvg(u_voxelTexture, ivec3(dim*posVoxelSpace), val);
	} else {
		val.a *= 1.f / 255.f;
		vec4 prev = imageLoad(u_voxelTexture, ivec3(dim*posVoxelSpace));
		vec3 avg = (prev.rgb * prev.a + val.rgb * val.a) / (prev.a + val.a);
		val = vec4(avg, prev.a + val.a);
		imageStore(u_voxelTexture, ivec3(dim*posVoxelSpace), val);
	}
}