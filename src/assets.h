#pragma once

#include <glad/glad.h>

#include <unordered_map>

#include "utils.h"
#include "window.h"

static inline string getAssetsFolder() {
	return "../assets/";
}

struct Asset {
	vector<string> files;
	double loadTime = -100.0;

	virtual void loadImpl() {

	}

	void load() {
		double currTime = System::time();
		if (currTime - loadTime > 1.0) {
			loadImpl();
			loadTime = currTime;
		}
	}
};

struct AssetStore {
	std::unordered_map<string, Asset*> assets;
	std::unordered_map<string, Asset*> deps;

	AssetStore() {
		init();
	}

	void init();

	void update();

	template<typename T>
	T *add(const string &name, T* asset) {
		for (string dep: asset->files) {
			deps[dep] = (Asset*)asset;
		}
		assets[name] = (Asset*)asset;
		return asset;
	}

	Asset *get(const string &name) {
		return assets[name];
	}

};

struct Texture {
	u32 id;
	u32 type;

	static Texture *init3D(int size, bool mipmap) {
		Texture *t = new Texture();
		u32 type = GL_TEXTURE_3D;
		u32 id;
		glGenTextures(1, &id);
		glBindTexture(type, id);

		glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

		if (mipmap) {
			glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		} else {
			glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		u32 levels = mipmap ? (u32)std::floor(std::log2(size))+1 : 1;
		LOG("3D texture - size: ", size, " levels: ", levels);
		glTexStorage3D(type, levels, GL_RGBA8, size, size, size);
		t->id = id;
		t->type = type;
		return t;
	}

	static Texture *shadowMap(int width, int height) {
		Texture *t = new Texture();
		t->type = GL_TEXTURE_2D;
		glGenTextures(1, &t->id);
		t->bind(0);
		glTexImage2D(t->type, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(t->type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(t->type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(t->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
		glTexParameteri(t->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); 
		return t;
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

struct Framebuffer {
	int width, height;
	u32 id;
	Texture *t;

	Framebuffer(int width, int height): width(width), height(height) {
		glGenFramebuffers(1, &id);
	}

	void setViewport() {
		glViewport(0, 0, width, height);
	}

	void bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, id);
	}

	void dispose() {
		glDeleteFramebuffers(1, &id);
	}

	static Framebuffer *shadowMap(int width, int height) {
		Framebuffer *buffer = new Framebuffer(width, height);
		buffer->t = Texture::shadowMap(width, height);

		buffer->bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, buffer->t->id, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		Framebuffer::reset();

		return buffer;
	}

	static void reset() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

};

struct Shader: Asset {
	int id = 0;
	string vpath, gpath, fpath;

	Shader() {}
	Shader(const string &cpath);
	Shader(const string &vpath, const string &fpath);
	Shader(const string &vpath, const string &gpath, const string &fpath);

	void loadImpl();

	void bind();

	void dispatch(u32 sx, u32 sy, u32 sz);

	void set(u32 uid, const int *value, int count) {
		glUniform1iv(uid, count, value);
	}

	void set(u32 uid, const float *value, int count) {
		glUniform1fv(uid, count, value);
	}

	void set(u32 uid, const vec2* value, int count) {
		glUniform2fv(uid, count, (const float*)value);
	}

	void set(u32 uid, const vec3* value, int count) {
		glUniform3fv(uid, count, (const float *)value);
	}

	void set(u32 uid, const vec4* value, int count) {
		glUniform4fv(uid, count, (const float *)value);
	}

	void set(u32 uid, const mat2* value, int count) {
		glUniformMatrix2fv(uid, count, GL_FALSE, (const float *)value);
	}

	void set(u32 uid, const mat3* value, int count) {
		glUniformMatrix3fv(uid, count, GL_FALSE, (const float *)value);
	}

	void set(u32 uid, const mat4* value, int count) {
		glUniformMatrix4fv(uid, count, GL_FALSE, (const float *)value);
	}

	void set(u32 uid, int value) {
		glUniform1i(uid, value);
	}

	void set(u32 uid, float value) {
		glUniform1f(uid, value);
	}

	void set(u32 uid, const vec2& value) {
		glUniform2fv(uid, 1, &value[0]);
	}

	void set(u32 uid, const vec3& value) {
		glUniform3fv(uid, 1, &value[0]);
	}

	void set(u32 uid, const vec4& value) {
		glUniform4fv(uid, 1, &value[0]);
	}

	void set(u32 uid, const mat2& value) {
		glUniformMatrix2fv(uid, 1, GL_FALSE, &value[0][0]);
	}

	void set(u32 uid, const mat3& value) {
		glUniformMatrix3fv(uid, 1, GL_FALSE, &value[0][0]);
	}

	void set(u32 uid, const mat4& value) {
		glUniformMatrix4fv(uid, 1, GL_FALSE, &value[0][0]);
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

	static const char *getUniformName(const char *name, int index);

	template <class T>
	void setIndex(const char *name, int index, T value) {
		set(getUniformName(name, index), value);
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


Mesh *loadMesh(const string &path);

Texture loadTexture(const string &path);

void genMipmapLevel(Shader *program, Texture *src, Texture *dest, int level, int size, bool aniso, int dir);

void genMipmap(Shader *program, Texture *texture, int size, bool aniso, int dir);

void genMipmap(Shader *program, Texture *texture, int size);
