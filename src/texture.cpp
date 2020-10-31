
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "assets.h"
#include "window.h"

Texture *createTexture(u8 *data, int width, int height, bool mipmap, bool aniso) {
	u32 id;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	if (mipmap) {
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 

	if (aniso) {
		float an;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &an);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, (int)an); 
	}

	Texture *t = new Texture();
	t->id = id;
	t->type = GL_TEXTURE_2D;
	return t;
}

Texture *loadTexture(const char *path) {
	if (path == nullptr) return nullptr;

	auto startTime = System::time();

	i32 width, height, cpp;

	stbi_set_flip_vertically_on_load(true);
	u8 *data = stbi_load(path, &width, &height, &cpp, 3);
	if (data == nullptr) {
		LOG("cannot open texture:", path);
		return nullptr;
	}

	Texture *t = createTexture(data, width, height);

	stbi_image_free(data);

	auto endTime = System::time();
	LOG(endTime - startTime, "loaded texture:", path);
	return t;
}

Texture *white = nullptr;
Texture *black = nullptr;
Texture *normal = nullptr;

Texture *colorTexture(float r, float g, float b) {
	r = glm::clamp(r*256.f, 0.f, 255.f);
	g = glm::clamp(g*256.f, 0.f, 255.f);
	b = glm::clamp(b*256.f, 0.f, 255.f);
	u8 color[] = {(u8)r, (u8)g, (u8)b};
	return createTexture(color, 1, 1, false, false);
}

Texture *whiteTexture() {
	if (!white) white = colorTexture(1.f, 1.f, 1.f);
	return white;
}

Texture *blackTexture() {
	if (!black) black = colorTexture(0.f, 0.f, 0.f);
	return black;
}

Texture *normalTexture() {
	if (!normal) normal = colorTexture(.5f, .5f, 1.f);
	return normal;
}

void genMipmapLevel(Shader *program, Texture *src, Texture *dest, int level, int size, bool aniso, int dir) {
	program->bind();
	program->set("u_level", std::max(0.0f, (float)(level-1)));
	program->set("u_src", 0);
	src->bind(0);
	program->set("u_dest", 1);
	program->set("u_aniso", aniso);
	program->set("u_dir", dir);
	glBindImageTexture(1, dest->id, level, GL_TRUE, 0, GL_WRITE_ONLY, dest->storageType);

	program->dispatch(size, size, size);
}

void genMipmap(Shader *program, Texture *texture, int size, bool aniso, int dir) {
	int level = 1;
	while (size > 0) {
		genMipmapLevel(program, texture, texture, level, size, aniso, dir);
		size /= 2;
		level++;
	}
}

void genMipmap(Shader *program, Texture *texture, int size) {
	genMipmap(program, texture, size, false, 0);
}