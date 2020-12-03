#pragma once

#include <glad/glad.h>

#include <unordered_map>

#include "utils.h"
#include "window.h"

static inline string getAssetsFolder() {
	return "../assets/";
}

/* Base class for all assets types, handles loading and disposing of assets,
 * as well as file dependencies */
struct Asset {
	vector<string> files; /* list of files the asset depends on */
	double loadTime = -100.0; /* last time the asset was loaded */

	virtual void loadImpl() {

	}

	bool checkTime() {
		return System::time() - loadTime > 0.5;
	}

	void updateLoadTime() {
		loadTime = System::time();
	}

	void load() {
		loadTime = System::time();
		loadImpl();
	}

	void checkAndLoad() {
		double currTime = System::time();
		if (currTime - loadTime > 0.5) {
			loadImpl();
			loadTime = currTime;
		}
	}

	virtual void dispose() {
		
	}
};

/* Handles automatic reload of modified assets */
struct AssetStore {
	std::unordered_map<string, Asset*> assets; /* loaded assets */
	std::unordered_map<string, Asset*> deps; /* map filename to asset */

	AssetStore() {
		init();
	}

	void init();

	void update();

	template<typename T>
	T *add(const string &name, T* asset) {
		for (string dep: asset->files) {
			/* when the asset is added we set all of its files pointing to the asset
			in order to update it when the files change */
			deps[dep] = (Asset*)asset;
		}
		assets[name] = (Asset*)asset;
		return asset;
	}

	Asset *get(const string &name) {
		return assets[name];
	}

};

/* Texture wrapper class */
struct Texture: Asset {
	u32 id = 0;
	u32 type;
	u32 format;
	u32 storageType;

	/* utility function to create 3D textures */
	static Texture *init3D(int size, bool mipmap, bool hdr) {
		Texture *t = new Texture();
		t->type = GL_TEXTURE_3D;
		t->format = GL_RGBA;
		/* use different storage for hdr */
		t->storageType = hdr ? GL_RGBA16F : GL_RGBA8; 

		glGenTextures(1, &t->id);
		glBindTexture(t->type, t->id);

		glTexParameteri(t->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(t->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(t->type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

		if (mipmap) {
			glTexParameteri(t->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		} else {
			glTexParameteri(t->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		glTexParameteri(t->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		u32 levels = mipmap ? (u32)std::floor(std::log2(size))+1 : 1;
		LOG("3D texture - size: ", size, " levels: ", levels);
		glTexStorage3D(t->type, levels, t->storageType, size, size, size);

		return t;
	}

	/* utility function to create a shadow map texture */
	static Texture *shadowMap(int width, int height) {
		Texture *t = new Texture();
		t->type = GL_TEXTURE_2D;
		t->format = GL_DEPTH_COMPONENT; /* only need depth */
		t->storageType = GL_FLOAT;
		glGenTextures(1, &t->id);
		t->bind(0);
		glTexImage2D(t->type, 0, t->format, width, height, 0, t->format, t->storageType, NULL);
		glTexParameteri(t->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(t->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(t->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
		glTexParameteri(t->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); 
		return t;
	}

	void bind(int channel) {
		glActiveTexture(GL_TEXTURE0 + channel);
		glBindTexture(type, id);
	}

	void clear(const vec4& value) {
		glClearTexImage(id, 0, format, GL_FLOAT, &value[0]);
	}

	void dispose() {
		glDeleteTextures(1, &id);
		id = 0;
	}

	~Texture() {
		dispose();
	}
};

/* Framebuffer wrapper class */
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

/* Shader wrapper class */
struct Shader: Asset {
	int id = 0;
	string vpath, gpath, fpath;

	string header = "";

	/* constructors for compute shaders and standard shaders */
	Shader() {}
	Shader(const string &cpath, const string &header);
	Shader(const string &vpath, const string &fpath, const string &header);
	Shader(const string &vpath, const string &gpath, const string &fpath, const string &header);

	void loadImpl();

	/* binds shader */
	void bind();

	/* used to run compute shaders */
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

	/* utility to set uniform arrays */
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

/* Mesh class, stores vertices and indices and loads everything to gpu */
struct Mesh: Asset {
	vector<Vertex> vertices;
	vector<u32> indices;
	u32 vao = 0, vbo = 0, ebo = 0;

	Mesh(vector<Vertex> &vertices, vector<u32> &indices);

	Mesh(Mesh &&);

	~Mesh();

	Mesh &operator=(Mesh &&);

	Mesh(const Mesh &copy) = delete;
	Mesh &operator=(const Mesh &) = delete;


	void bind();

	static void reset();

	void drawNoBind();

	void draw();

	void setup();

	void dispose();
};


/* loads only one group from an OBJ file handle*/
Mesh *loadMesh(void *obj_handle, int group_id);

/* loads a OBJ file as a single mesh */
Mesh *loadMesh(const string &path);

/* loads a texture from path */
Texture *loadTexture(const char *path);

/* creates a texture from memory */
Texture *createTexture(u8 *data, int width, int height, bool mipmap = true, bool aniso = true);

Texture *whiteTexture();
Texture *blackTexture();
Texture *normalTexture();

/* generates one mipmap level for a 3D texture using the given compute shader */
void genMipmapLevel(Shader *program, Texture *src, Texture *dest, int level, int size, bool aniso, int dir);

/* generates a mipmap for a 3D anisotropic texture using the given compute shader */
void genMipmap(Shader *program, Texture *texture, int size, bool aniso, int dir);

/* generates a mipmap for a 3D texture using the given compute shader */
void genMipmap(Shader *program, Texture *texture, int size);
