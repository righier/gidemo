
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "assets.h"
#include "window.h"

Texture loadTexture(const string &path) {

	auto startTime = System::time();

	u32 id;
	i32 width, height, cpp;

	stbi_set_flip_vertically_on_load(true);
	u8 *data = stbi_load(path.c_str(), &width, &height, &cpp, 3);

	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 

	float aniso;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &aniso);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, (int)aniso); 

	stbi_image_free(data);

	Texture t;
	t.id = id;
	t.type = GL_TEXTURE_2D;

	auto endTime = System::time();
	LOG(endTime - startTime, "loaded texture:", path);
	return t;
}

void genMipmapLevel(Shader *program, Texture *src, Texture *dest, int level, int size, bool aniso, int dir) {
	program->bind();
	program->set("u_level", std::max(0.0f, (float)(level-1)));
	program->set("u_src", 0);
	src->bind(0);
	program->set("u_dest", 1);
	program->set("u_aniso", aniso);
	program->set("u_dir", dir);
	glBindImageTexture(1, dest->id, level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

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