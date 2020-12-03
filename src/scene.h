#pragma once

#include "light.h"
#include "object.h"
#include "particlesystem.h"

/* Scene class, stores all objects, particle systems and lights */
struct Scene {
	vector<Object> objects;
	vector<ParticleSystem> particles;

	SpotLight light;

	/* add a particle system to the scene */
	void add(ParticleSystem p) {
		particles.push_back(p);
	}

	/* add a object to the scene */
	void add(Object obj) {
		objects.push_back(obj);
	}

	/* draw all objects using the given shader */
	void drawObjects(double time, Shader *shader, bool useMaterials = true) {
		UNUSED(time);
		light.upload(shader);

		for (auto &obj: objects) {
			obj.draw(shader, useMaterials);
		}
	}

	/* draw all particle systems */
	void drawParticles(double time, Shader *shader) {
		for (auto &p: particles) {
			/* use the particle system address as seed for random values */
			p.draw(shader, time, (int)reinterpret_cast<unsigned long long>(&p));
		}
	}

	/* returns an object matching the given name */
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

/* loads all meshes and texture from an OBJ file */
void loadScene(Scene &scene, const char *path);
