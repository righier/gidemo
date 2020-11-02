#pragma once

#include "light.h"
#include "object.h"
#include "particlesystem.h"

struct Scene {
	vector<Object> objects;
	vector<ParticleSystem> particles;

	SpotLight light;

	void add(ParticleSystem p) {
		particles.push_back(p);
	}

	void add(Object obj) {
		objects.push_back(obj);
	}

	void draw(double time, Shader *shader, bool useMaterials = true) {
		light.upload(shader);

		// shader->set("u_nPointLights", (int)plights.size());
		// shader->set("u_pointLights", (vec3*)&plights[0], plights.size()*3);

		for (auto &obj: objects) {
			obj.draw(shader, useMaterials);
		}

		for (auto &p: particles) {
			p.draw(time);
		}
	}

	Object *get(const string &name) {
		for (Object &o: objects) {
			if (o.name == name) {
				return &o;
			}
		}
		LOG("OBJECT NOT FOUND:", name);
		return nullptr;
	}
};

void loadScene(Scene &scene, const char *path);
