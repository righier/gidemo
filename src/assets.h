#pragma once

#include <glad/glad.h>

#include <unordered_map>

#include "utils.h"

struct Texture {
	u32 id;
	u32 type;

	void init3D(int size) {
		type = GL_TEXTURE_3D;
		glGenTextures(1, &id);
		glBindTexture(type, id);

		glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

		glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		u32 levels = (u32)std::ceil(std::log2(size))+1;
		levels = 7;
		LOG("size: ", size, " levels: ", levels);
		glTexStorage3D(type, levels, GL_RGBA8, size, size, size);
	}

	void bind(int channel) {
		glActiveTexture(GL_TEXTURE0 + channel);
		glBindTexture(type, id);
	}

	void clear(const vec4& value) {
		glClearTexImage(id, 0, GL_RGBA, GL_FLOAT, &value[0]);
	}

	void dispose() {
		glDeleteTextures(1, &id);
	}
};

struct Shader {
	int id = 0;

	Shader() {}
	Shader(const string &vpath, const string &fpath);
	Shader(const string &vpath, const string &gpath, const string &fpath);

	void bind();

	void set(u32 id, const int *value, int count) {
		glUniform1iv(id, count, value);
	}

	void set(u32 id, const float *value, int count) {
		glUniform1fv(id, count, value);
	}

	void set(u32 id, const vec2* value, int count) {
		glUniform2fv(id, count, (const float*)value);
	}

	void set(u32 id, const vec3* value, int count) {
		glUniform3fv(id, count, (const float *)value);
	}

	void set(u32 id, const vec4* value, int count) {
		glUniform4fv(id, count, (const float *)value);
	}

	void set(u32 id, const mat2* value, int count) {
		glUniformMatrix2fv(id, count, GL_FALSE, (const float *)value);
	}

	void set(u32 id, const mat3* value, int count) {
		glUniformMatrix3fv(id, count, GL_FALSE, (const float *)value);
	}

	void set(u32 id, const mat4* value, int count) {
		glUniformMatrix4fv(id, count, GL_FALSE, (const float *)value);
	}

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

	template <class T>
	void set(const char *name, T value, u32 count) {
		u32 uid = glGetUniformLocation(this->id, name);
		set(uid, value, count);
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

struct AssetStore {
	std::unordered_map<string, void*> assets;

	void add(const string &name, void* asset) {
		assets[name] = asset;
	}

	void *get(const string &name) {
		return assets[name];
	}
};


Mesh *loadMesh(const string &path);


Texture loadTexture(const string &path);

// Texture loadTexture(string path, TextureFilter filter, TextureWrap wrap, bool mipmap);
// Texture loadTextureArray(string path, int depth, TextureFilter filter, TextureWrap wrap, bool mipmap);
// Texture loadCubemap(string path, TextureFilter filter, TextureWrap wrap, bool mipmap);
