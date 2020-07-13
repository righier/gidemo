#pragma once

#include "light.h"
#include "object.h"

struct Scene {
	vector<Object> objects;

	SpotLight light;

	void add(Object obj) {
		objects.push_back(obj);
	}

	void draw(Shader *shader) {
		light.upload(shader);

		// shader->set("u_nPointLights", (int)plights.size());
		// shader->set("u_pointLights", (vec3*)&plights[0], plights.size()*3);

		for (auto &obj: objects) {
			obj.draw(shader);
		}
	}
};