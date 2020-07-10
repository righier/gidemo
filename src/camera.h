#pragma once

#include "utils.h"

struct Camera {
	float fov, ratio, near, far;

	vec3 pos;
	float hrot, vrot;

	mat4 proj;
	mat4 view;
	mat4 final;

	Camera(float fov, float ratio, float near, float far) {
		this->fov = fov;
		this->ratio = ratio;
		this->near = near;
		this->far = far;
		this->pos = vec3(0, 0, 0);
		this->hrot = 0.f;
		this->vrot = 0.f;

		update();
	}

	void move(const vec3 &dpos) {
		pos += dpos;
	}

	void rotate(float dh, float dv) {
		hrot = glm::mod(hrot+dh, pi * 2.f);
		vrot = glm::clamp(vrot+dv, -pi * .5f, pi * .5f);
	}

	void updateView() {
		view = glm::identity<mat4>();
		view = glm::rotate(view, -vrot, vec3(1.f, 0.f, 0.f));
		view = glm::rotate(view, -hrot, vec3(0.f, 1.f,0.f));
		view = glm::translate(view, -pos);
	}

	void updateProj() {
		proj = glm::perspective(fov, ratio, near, far);
	}

	void update() {
		updateProj();
		updateView();
		final = proj * view;
	}

};