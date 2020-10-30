
#include "window.h"
#include "utils.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

namespace Window {

	GLFWwindow *window = NULL;
	std::string title;
	int w, h;
	int type;
	bool vsync;

	void *getNativeWindow() {
		return window;
	}

	void callbackResize(GLFWwindow *_window, int width, int height) {
		UNUSED(_window);
		w = width;
		h = height;
		glViewport(0, 0, width, height);
		glScissor(0, 0, width, height);
	}

	void create(int width, int height, std::string _title, int _type, bool _vsync) {
		w = width;
		h = height;
		Window::title = _title;
		Window::vsync = _vsync;
		Window::type = _type;

		GLFWmonitor *monitor = NULL;

		if (type == WINDOWED) {
		// nothing to do
		} else if (type == BORDERLESS) {
			monitor = glfwGetPrimaryMonitor();
			auto mode = glfwGetVideoMode(monitor);
			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
			w = mode->width;
			h = mode->height;
		} else if (type == FULLSCREEN) {
			monitor = glfwGetPrimaryMonitor();
		} else {
			// TODO: unknown type
		}

		window = glfwCreateWindow(w, h, title.c_str(), monitor, NULL);

		if (window == NULL) {
			std::cout << "Failed to create window" << std::endl;
		}
		glfwMakeContextCurrent(window);
		glfwSetFramebufferSizeCallback(window, callbackResize);

		glfwSwapInterval(vsync);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			std::cout << "Failed to initialize GLAD" << std::endl;
		}

		const u8 *v = glGetString(GL_SHADING_LANGUAGE_VERSION);

		std::string glslVersion = "#version ";
		glslVersion.push_back(v[0]);
		glslVersion.push_back(v[2]);
		glslVersion.push_back(v[3]);

		LOG(glslVersion);


		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init(glslVersion.c_str());
	}

	void destroy() {
		glfwDestroyWindow(window);
	}


	bool first_run = true;
	void update() {

		if (!first_run) {
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(window);
		} else {
			first_run = false;
		}

		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();	
	}

	bool shouldClose() {
		return glfwWindowShouldClose(window);
	}

	void setShouldClose(bool value) {
		glfwSetWindowShouldClose(window, value);
	}

	int getWidth() {
		return w;
	}

	int getHeight() {
		return h;
	}

	vec2 getSize() {
		int ww, hh;
		glfwGetFramebufferSize(window, &ww, &hh);
		return vec2((float)ww, (float)hh);
	}

	int getType() {
		return type;
	}

	bool getVSyncStatus() {
		return vsync;
	}

	void setVSyncStatus(bool value) {
		vsync = value;
		glfwSwapInterval(value);
	}
}

namespace Input {
	int getKey(int id) {
		return glfwGetKey(Window::window, id) == GLFW_PRESS;
	}

	int getMouseButton(int id) {
		return glfwGetMouseButton(Window::window, id) == GLFW_PRESS;
	}

	vec2 getMousePos() {
		double x, y;
		glfwGetCursorPos(Window::window, &x, &y);
		return vec2((float)x, (float)y);
	}

	void setMousePos(const vec2& pos) {
		glfwSetCursorPos(Window::window, pos.x, pos.y);
	}

	void showCursor(bool state) {
		u32 val = state ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED;
		glfwSetInputMode(Window::window, GLFW_CURSOR, val);
		ImGui_setIgnoreInput(!state);
	}
}

namespace System {
	void init(int gl_major, int gl_minor) {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gl_major);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gl_minor);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);	
		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);  
	#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif
	}

	void exit(int code) {
		glfwTerminate();
		std::exit(code);
	}

	double time() {
		return glfwGetTime();
	}

#ifdef _WIN32
#include <windows.h>
  void sleep(double t) {
    Sleep((System::DWORD)(t * 1000));
  }
#else
#include <unistd.h>
  void sleep(double t) {
    usleep(t * 1000000);
  }
#endif

}
