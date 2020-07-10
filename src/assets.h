#pragma once

#include <glad/glad.h>

#include "utils.h"

enum TextureFilter {
	TEXTURE_FILTER_NEAREST,
	TEXTURE_FILTER_LINEAR
};

enum TextureWrap {
	TEXTURE_WRAP_REPEAT,
	TEXTURE_WRAP_MIRROR,
	TEXTURE_WRAP_EDGE,
	TEXTURE_WRAP_BORDER
};

struct Texture {
	int id;
	u32 type;

	void bind(int channel) {
		glActiveTexture(GL_TEXTURE0 + channel);
		glBindTexture(type, id);
	}
};

struct Shader {
	int id;

	Shader(const string &vpath, const string &fpath);

	void bind();

	void set(u32 id, int value) {
		glUniform1i(id, value);
	}

	void set(u32 id, float value) {
		glUniform1f(id, value);
	}

	void set(u32 id, const vec2& value) {
		glUniform2fv(id, 1, &value[0]);
	}

	void set(u32 id, const vec3& value) {
		glUniform3fv(id, 1, &value[0]);
	}

	void set(u32 id, const vec4& value) {
		glUniform4fv(id, 1, &value[0]);
	}

	void set(u32 id, const mat2& value) {
		glUniformMatrix2fv(id, 1, GL_FALSE, &value[0][0]);
	}

	void set(u32 id, const mat3& value) {
		glUniformMatrix3fv(id, 1, GL_FALSE, &value[0][0]);
	}

	void set(u32 id, const mat4& value) {
		glUniformMatrix4fv(id, 1, GL_FALSE, &value[0][0]);
	}

	template <class T>
	void set(const char *name, T value) {
		u32 uid = glGetUniformLocation(this->id, name);
		set(uid, value);
	}

	void dispose();
};

struct Vertex {
    vec3 p; //position
    vec2 uv; //texture uv
    vec3 n; //normal
    vec3 t; //tangent
    vec3 b; //bitangent
};

struct Mesh {
	vector<Vertex> vertices;
	vector<u32> indices;
	u32 vao = 0, vbo = 0, ebo = 0;

	Mesh(vector<Vertex> &vertices, vector<u32> &indices);

	Mesh(Mesh &&);

	~Mesh();

	Mesh &operator=(Mesh &&);

	Mesh(const Mesh &copy) = delete;
	Mesh &operator=(const Mesh &) = delete;


	void draw();

	void setup();

	void dispose();
};


Mesh loadMesh(const string &path);


Texture loadTexture(const string &path);

// Texture loadTexture(string path, TextureFilter filter, TextureWrap wrap, bool mipmap);
// Texture loadTextureArray(string path, int depth, TextureFilter filter, TextureWrap wrap, bool mipmap);
// Texture loadCubemap(string path, TextureFilter filter, TextureWrap wrap, bool mipmap);
