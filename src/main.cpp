#include <glad/glad.h>

#include "window.h"
#include "utils.h"
#include "camera.h"
#include "assets.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

int main() {
	System::init(4, 6);

	Window::create(800, 600, "title", Window::WINDOWED, false);

	Window::update();

	double oldTime = System::time();
	int updateCount = 0;

	Shader shader = Shader("../assets/shaders/flat.vert", "../assets/shaders/flat.frag");

	Mesh bunny = loadMesh("../assets/models/bunny.obj");

	Mesh cube = loadMesh("../assets/models/cube.obj");

	Texture testuv = loadTexture("../assets/textures/test.jpg");

	Texture normal = loadTexture("../assets/textures/leather/normal.jpg");

	Camera cam(glm::radians(60.f), (float)Window::getWidth() / (float)Window::getHeight(), .1f, 100.f);

	cam.pos = vec3(0.f, 0.f, 10.f);

	mat4 id = glm::identity<mat4>();

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_SCISSOR_TEST);
	// glCullFace(GL_FRONT_AND_BACK);
	// glFrontFace(GL_CCW);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	bool showCursor = false;
	int lockPrev = false;
	Input::showCursor(showCursor);
	vec2 oldMousePos = Input::getMousePos();

	double prevTime = System::time();

	float rotation = 0;

	while(!Window::shouldClose()) {
		double time = System:: time();
		float timeDelta = (float)(time - prevTime);
		prevTime = time;

		if (time - oldTime >= 1.0) {
			double delta = time - oldTime;
			LOG("FPS:", updateCount, "--  ms:", (delta / updateCount * 1000 / (time - oldTime)));

			oldTime += 1.0;
			updateCount = 0;
		}

		if (Input::getKey(KEY_ESCAPE)) {
			Window::setShouldClose(true);
		}

		int lock = Input::getKey(KEY_SPACE);
		if (lock & !lockPrev) {
			showCursor = !showCursor;
			//LOG("TOGGLE CURSOR:", showCursor);
			Input::showCursor(showCursor);
		}
		lockPrev = lock;

		vec2 winSize = Window::getSize();

		vec2 mousePos = Input::getMousePos();

		float mouseSens = 1.f / 500;

		vec2 mouseDelta = -(mousePos - oldMousePos) * mouseSens;
		if (showCursor) mouseDelta = vec2(0, 0);

		oldMousePos = mousePos;

		vec3 move(0,0,0);

		if (Input::getKey(KEY_W)) move.z -= 1;
		if (Input::getKey(KEY_S)) move.z += 1;
		if (Input::getKey(KEY_A)) move.x -= 1;
		if (Input::getKey(KEY_D)) move.x += 1;
		if (Input::getKey(KEY_Q)) move.y -= 1;
		if (Input::getKey(KEY_E)) move.y += 1;

		float moveSpeed = 3.f;
		move = move * moveSpeed * (float)timeDelta;

		move = glm::rotateY(move, cam.hrot);


		cam.ratio = winSize.x / winSize.y;

		cam.rotate(mouseDelta.x, mouseDelta.y);
		cam.move(move);

		cam.update();

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		rotation = fmod(rotation + timeDelta * 360.f / 30.f, 360.f);

		mat4 scale = glm::scale(id, vec3(1,1,1) / 100.f);
		mat4 rotate = glm::rotate(id, glm::radians(rotation), vec3(0, 1, 0));

		shader.bind();
		shader.set("u_project", cam.final);
		shader.set("u_rotate", rotate);
		shader.set("u_transform", rotate);

		bunny.draw();


		mat4 moveCube = glm::translate(id, vec3(3, 0, 0));
		shader.set("u_transform", moveCube * rotate);
		shader.set("u_tex", 0);
		normal.bind(0);
		cube.draw();

		updateCount++;

		ImGui::Begin("Demo window");
		ImGui::Button("Hello!");
		ImGui::End();

		Window::update();
	}

	System::exit(0);
	return 0;
}
