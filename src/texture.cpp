
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, aniso); 

	stbi_image_free(data);

	Texture t;
	t.id = id;
	t.type = GL_TEXTURE_2D;

	auto endTime = System::time();
	LOG(endTime - startTime, "loaded texture:", path);
	return t;
}