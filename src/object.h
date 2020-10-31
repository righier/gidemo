#pragma once

#include "utils.h"
#include "assets.h"

struct Material {
	vec3 diffuse = vec3(1);
	vec3 emission = vec3(1);
	float metal = 1.0;
	float rough = 1.0;
	float emissionScale = 1.0;

	Texture *diffuseMap = nullptr;
	Texture *bumpMap = nullptr;
	Texture *specMap = nullptr;
	Texture *emissionMap = nullptr;

	void bind(Shader *shader) {
		shader->set("u_diffuse", diffuse);
		shader->set("u_emission", emission);

		shader->set("u_metal", metal);
		shader->set("u_rough", rough);
		shader->set("u_emissionScale", emissionScale);

		shader->set("u_useMaps", diffuseMap != nullptr);

		if (diffuseMap) {
			diffuseMap->bind(10);
			bumpMap->bind(11);
			specMap->bind(12);
			emissionMap->bind(13);
		}

		shader->set("u_diffuseMap", 10);
		shader->set("u_bumpMap", 11);
		shader->set("u_specMap", 12);
		shader->set("u_emissionMap", 13);
	}

	Material() { }

	Material(vec3 diffuse, vec3 emission, float metal, float rough, float emissionScale = 1.0): 
	diffuse(diffuse), 
	emission(emission), 
	metal(metal), 
	rough(rough),
	emissionScale(emissionScale)
	{ }

	Material(vec3 diffuse): 
	Material(diffuse, vec3(0), 0.f, 1.f) 
	{ }

	Material(Texture *dMap, Texture *bMap, Texture *sMap, Texture *eMap, float emissionScale = 1.0): 
	diffuseMap(dMap),
	bumpMap(bMap),
	specMap(sMap),
	emissionMap(eMap),
	emissionScale(emissionScale)
	{ }
};

struct Object {
	string name;
	Mesh *mesh;
	Material mat;

	vec3 pos;
	vec3 scale;
	quat rot;

	Object(string name, Mesh *mesh, Material mat, vec3 pos, vec3 scale, quat rot): 
	name(name), mesh(mesh), mat(mat), pos(pos), scale(scale), rot(rot) {

	}

	mat4 genMatrix() {
		mat4 t = glm::identity<mat4>();
		t = glm::translate(t, pos);
		t = glm::scale(t, scale);
		t = t * glm::toMat4(rot);
		return t;
	}

	void draw(Shader *shader) {
		mat4 transform = genMatrix();
		shader->set("u_transform", transform);
		mat.bind(shader);
		mesh->draw();
	}
};