
#include "utils.h"
#include "assets.h"

struct Material {
	static Shader shader;

	vec3 color;
	float rough;
	float metal;

	Material(const vec3 &color, float rough, float metal) {
		this->color = color;
		this->rough = rough;
		this->metal = metal;
	}

	void bind(const mat4 &viewMatrix, const mat4 &modelMat) {
		shader.bind();
		shader.set("u_color", color);
		shader.set("u_rough", rough);
		shader.set("u_metal", metal);
		shader.set("u_view_mat", viewMat);
		shader.set("u_model_mat", modelMat);
	}
};

struct Entity {
	vec3 pos;
	vec3 scale;
	quat rot;

	Material &mat;
	Mesh *mesh;

	Entity(const vec3 &pos, const vec3 &scale, const quat &rot, const Material &mat, Mesh &mesh) {
		this->pos = pos;
		this->scale = scale;
		this->rot = rot;
		this->mat = mat;
		this->mesh = mesh;
	}

	public render(const mat4 &viewMatrix) {
		mat4 modelMat = transMat * rotMat * scaleMat;
		mat.bind(viewMatrix, modelMat);
		mesh.draw();
	}
};