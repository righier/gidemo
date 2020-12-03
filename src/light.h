#pragma once 

#include "utils.h"
#include "assets.h"

/*
struct DirLight {
	vec3 dir;
	vec3 color;

	DirLight(vec3 dir, vec3 col): dir(dir), color(col) {

	}
};

struct PointLight {
	vec3 pos;
	vec3 color;
	vec3 atten;

	PointLight(vec3 pos, vec3 col, vec3 atten): pos(pos), color(col), atten(atten) {
		
	}
};
*/

/* Simple Spotlight class */
struct SpotLight {
	vec3 pos; /* position */
	vec3 dir; /* direciton */
	vec3 color; /* color */
	float angle; /* aperture angle */
	float smooth; /* aperture of the interpolation angle between light and no light */

	SpotLight() {

	}

	SpotLight(vec3 pos, vec3 dir, vec3 color, float angle, float smooth):
	pos(pos), dir(dir), color(color), angle(angle), smooth(smooth) {

	}

	/* generates projection matrix from the light point of view */
	mat4 getProjectionMatrix() {
		vec3 up = dir + vec3(0,0,1);
		up = normalize(cross(up, dir)); /* orthogonal vector to dir */
		mat4 view = glm::lookAt(pos, pos+dir, up);
		mat4 proj = glm::perspective(glm::radians(angle), 1.f, 0.01f, 3.0f);
		return proj * view;
	}

	/* set all the uniforms for the light */
	void upload(Shader *shader) {
		shader->set("u_lightPos", pos);
		shader->set("u_lightDir", glm::normalize(dir));
		shader->set("u_lightColor", color);
		shader->set("u_lightInnerCos", cos(.5f * glm::radians(angle - smooth)));
		shader->set("u_lightOuterCos", cos(.5f * glm::radians(angle)));
	}
};