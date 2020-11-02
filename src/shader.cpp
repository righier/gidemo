#include <glad/glad.h>

#include "assets.h"

bool checkAndLogShaderError(u32 id) {
	GLint status, logSize;
	glGetShaderiv(id, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logSize);
		auto errorLog = new char[logSize];
		glGetShaderInfoLog(id, logSize, &logSize, errorLog);
		LOG(errorLog);
		delete[] errorLog;
		return false;
	} else return true;
}

bool checkAndLogProgramError(u32 id) {
	GLint status, logSize;
	glGetProgramiv(id, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logSize);
		auto errorLog = new char[logSize];
		glGetProgramInfoLog(id, logSize, &logSize, errorLog);
		LOG(errorLog);
		delete[] errorLog;
		return false;
	} else return true;
}

u32 loadShader(const string &path, GLenum type, const string &header) {
	if (path.size() == 0) return 0;

	string source = "#version 460 core\n" + header;
	LOG("LOAD: ", path);
	if (!readFile("../assets/shaders/" + path, source)) {
		LOG("Cannot find shader source file:", path);
		return 0;
	}

	u32 shader = glCreateShader(type);
	if (!shader) {
		LOG("Cannot create shader for:", path);
		return 0;
	}

	auto cstr = source.c_str();
	glShaderSource(shader, 1, &cstr, 0);
	glCompileShader(shader);

	if (!checkAndLogShaderError(shader)) {
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

u32 loadComputeShader(const string &cpath, const string &header) {
	LOG("load compute shader:", cpath);

	u32 comp = loadShader(cpath, GL_COMPUTE_SHADER, header);
	if (comp == 0) return 0;

	u32 id = glCreateProgram();

	glAttachShader(id, comp);
	glLinkProgram(id);

	if (!checkAndLogProgramError(id)) {
		LOG("ERROR COMPUTE LINK");
		glUseProgram(0);
		glDeleteProgram(id);
		id = 0;
	}

	glDetachShader(id, comp);
	glDeleteShader(comp);
	glUseProgram(0);
	return id;
}

u32 loadProgram(const string &vpath, const string &gpath, const string &fpath, const string &header) {
	LOG("load shader:", vpath, fpath);
	if (fpath=="") return loadComputeShader(vpath, header);

	u32 vert = loadShader(vpath, GL_VERTEX_SHADER, header);
	u32 geom = loadShader(gpath, GL_GEOMETRY_SHADER, header);
	u32 frag = loadShader(fpath, GL_FRAGMENT_SHADER, header);

	u32 id = 0;

	if (vert && frag) {
		LOG("compiled.");

		id = glCreateProgram();

		if (!id) {
			LOG("Cannot create shader program for:", vpath, fpath);
		}

		glAttachShader(id, vert);
		if (geom) glAttachShader(id, geom);
		glAttachShader(id, frag);
		glLinkProgram(id);

		if (!checkAndLogProgramError(id)) {
			LOG("ERROR");
			glUseProgram(0);
			glDeleteProgram(id);
			id = 0;
		} else {
			LOG("linked.");
			glDetachShader(id, vert);
			if (geom) glDetachShader(id, geom);
			glDetachShader(id, frag);
		}

	} else {
		return id;
		// exit(-1);
	}

	glDeleteShader(vert);
	if (geom) glDeleteShader(geom);
	glDeleteShader(frag);

	glUseProgram(0);
	return id;
}

char buffer[64];
const char *Shader::getUniformName(const char *name, int index) {
	snprintf(buffer, sizeof(buffer), "%s[%d]", name, index);
	return buffer;
}


Shader::Shader(const string &vpath, const string &fpath, const string &header): 
Shader(vpath, "", fpath, header) {

}

Shader::Shader(const string &cpath, const string &header):
Shader(cpath, "", "", header) {

}

Shader::Shader(const string &vpath, const string &gpath, const string &fpath, const string &header): 
id(0), vpath(vpath), gpath(gpath), fpath(fpath), header(header) {
	files.push_back(vpath);
	files.push_back(gpath);
	files.push_back(fpath);
	load();
	if (id == 0) {
		exit(-1);
	}
}

void Shader::loadImpl() {
	u32 newId = loadProgram(vpath, gpath, fpath, header);
	if (newId) {
		dispose();
		id = newId;
	}
}

void Shader::bind() {
	glUseProgram(id);
}

void Shader::dispatch(u32 sx, u32 sy, u32 sz) {
	glDispatchCompute(sx, sy, sz);
}

void Shader::dispose() {
	glUseProgram(0);
	if (id) glDeleteProgram(id);
}

