#pragma once

#include <string>

#include "utils.h"
#include "keys.h"

namespace Window {
	const int WINDOWED = 1;
	const int BORDERLESS = 2;
	const int FULLSCREEN = 3;

	void create(int width, int height, std::string title, int type, bool vsync);

	void destroy();

	void update();

	bool shouldClose();
	void setShouldClose(bool value);

	int getWidth();
	int getHeight();

	vec2 getSize();

	int getType();
	bool getVSyncStatus();
	void setVSyncStatus(bool value);

}

namespace Input {
	int getKey(int id);
	int getMouseButton(int id);
	vec2 getMousePos();
	void setMousePos(const vec2 &pos);
	void showCursor(bool state);
}

namespace System {
	void init(int gl_major, int gl_minor);
	void exit(int code);
	double time();
  void sleep(double t);
}

