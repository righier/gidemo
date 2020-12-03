#pragma once

#include <string>

#include "utils.h"
#include "keys.h"

/* namespace with utilities from handling the application window */
namespace Window {
	const int WINDOWED = 1;
	const int BORDERLESS = 2;
	const int FULLSCREEN = 3;

	/* create a new window with the given parameters */
	/* type can be WINDOWED, BORDERLESS, FULLSCREEN */
	void create(int width, int height, std::string title, int type, bool vsync);

	/* destroy a previously created window */
	void destroy();

	/* updates the window screen and input */
	void update();

	/* check if the x button has been pressed */
	bool shouldClose();
	/* override the close check */
	void setShouldClose(bool value);

	/* get window sizes */
	int getWidth();
	int getHeight();
	vec2 getSize();

	int getType();
	/* get vertical sync status */
	bool getVSyncStatus();
	/* set vertical sync status */
	void setVSyncStatus(bool value);

}

namespace Input {
	/* get a keyboard key status, 1 = pressed, 0 = not pressed */
	int getKey(int id);
	/* get the mouse button status */
	int getMouseButton(int id);
	/* get the mouse position */
	vec2 getMousePos();
	/* set the mouse position */
	void setMousePos(const vec2 &pos);
	/* true to show mouse cursor, false to hide */
	void showCursor(bool state);
}

namespace System {
	/* initialize glfw */
	void init(int gl_major, int gl_minor);
	/* exit application */
	void exit(int code);
	/* get current time in seconds */
	double time();
	/* sleep for the given number of seconds */
  void sleep(double t);
}

