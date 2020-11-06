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

	void drawObjects(double time, Shader *shader, bool useMaterials = true) {
		UNUSED(time);
		light.upload(shader);

		for (auto &obj: objects) {
			obj.draw(shader, useMaterials);
		}
	}

	void drawParticles(double time, Shader *shader) {
		for (auto &p: particles) {
			p.draw(shader, time, (int)reinterpret_cast<unsigned long long>(&p));
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
