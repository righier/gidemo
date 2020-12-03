#pragma once

#include "utils.h"

/* perspective camera utility class */
struct Camera {
	/* ratio is width / height */
	float fov, ratio, near, far;

	vec3 pos;
	/* rotation around Y and X axes */
	float hrot, vrot;

	mat4 proj; /* perspective projection matrix */
	mat4 view; /* translation and rotation matrix */
	mat4 final;

	Camera() {
		
	}

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

	/* moves the camera by dpos */
	void move(const vec3 &dpos) {
		pos += dpos;
	}

	/* rotates the camera around the Y and X axes */
	void rotate(float dh, float dv) {
		hrot = glm::mod(hrot+dh, pi * 2.f); 
		vrot = glm::clamp(vrot+dv, -pi * .5f, pi * .5f); /* lock at 90 degrees */
	}

	/* updates the view matrix */
	void updateView() {
		view = glm::identity<mat4>();
		/* the order is important */
		view = glm::rotate(view, -vrot, vec3(1.f, 0.f, 0.f));
		view = glm::rotate(view, -hrot, vec3(0.f, 1.f,0.f));
		view = glm::translate(view, -pos);
	}

	/* udates the proj matrix */
	void updateProj() {
		proj = glm::perspective(fov, ratio, near, far);
	}

	/* updates the final matrix */
	void update() {
		updateProj();
		updateView();
		final = proj * view;
	}

};