#include <glad/glad.h>

#include "window.h"
#include "utils.h"
#include "camera.h"
#include "assets.h"
#include "scene.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"


void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
	UNUSED(source);
	UNUSED(id);
	UNUSED(length);
	UNUSED(userParam);
  fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

Scene scene;
AssetStore assets;

// Texture *voxelTexture;
Texture *voxelTextures[6];

Shader *flatShader, *basicShader, *voxelizeShader, *visualizeShader, *voxelConeShader, *shadowShader;
Mesh *bunnyMesh, *cubeMesh, *planeMesh, *sphereMesh;

Shader *activeShader;

float shadowBias = 0.00007;
Framebuffer *shadowMap;

Camera cam;

int voxelCount = 128;
float voxelScale = 1.01f;
float offsetPos = 0.0f;
float offsetDist = 1.0f;

bool toggleVoxels = false;
float voxelLod = 0;

#ifdef _WIN32
#include <windows.h>
HANDLE changeHandle;
FILE_NOTIFY_INFORMATION infoBuffer[1024];
bool shaderFileChanged = false;
double lastShaderUpdate = 0;

DWORD WINAPI worker(LPVOID lpParam) {
	UNUSED(lpParam);
	while(true) {
		DWORD bytesReturned;
		if (ReadDirectoryChangesW(changeHandle, infoBuffer, sizeof(infoBuffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned, NULL, NULL)) {

			auto *p = &infoBuffer[0];
			do {
				std::wstring wname(p->FileName, p->FileNameLength/2);
				string name = ws2s(wname);
				name.resize(p->FileNameLength / 2);
				if (name == "voxelcone.vert" || name == "voxelcone.frag") {
					shaderFileChanged = true;
				}


				p += p->NextEntryOffset;
			} while(p->NextEntryOffset);
		}
	}
}

void initChangeHandle() {

	changeHandle = CreateFile("C:/users/ermanno/desktop/gidemo/assets/shaders", 
		FILE_LIST_DIRECTORY,
		FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
		NULL, 
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);

	if (changeHandle == INVALID_HANDLE_VALUE) {
		LOG("invalid handle error");
	} else {
		CreateThread( NULL, 0, worker, NULL, 0, NULL );
	}

}

void checkShaderChanges() {
	double currTime = System::time();
	if (currTime - lastShaderUpdate > 1.0 and shaderFileChanged) {
		voxelConeShader->load();
		lastShaderUpdate = currTime;
	}
	if (shaderFileChanged) shaderFileChanged = false;
}
#else
void initChangeHandle() {}
void checkShaderChanges() {}
#endif

void initVoxelize() {
	for (int i = 0; i < 6; i++) {
		voxelTextures[i] = Texture::init3D(voxelCount);
	}
}

void renderShadowMap() {

	shadowShader->bind();
	shadowShader->set("u_project", scene.light.getProjectionMatrix());

	shadowMap->bind();
	shadowMap->setViewport();

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);

	scene.draw(shadowShader);

	// glCullFace(GL_BACK);
}

void voxelize() {

	voxelizeShader->bind();


	Framebuffer::reset();
	glViewport(0, 0, voxelCount, voxelCount);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	// glEnable(GL_MULTISAMPLE);

	voxelizeShader->set("u_lightProj", scene.light.getProjectionMatrix());
	voxelizeShader->set("u_shadowBias", shadowBias);
 	shadowMap->t->bind(6);
	voxelizeShader->set("u_shadowMap", 6);
 
	voxelizeShader->set("u_voxelScale", voxelScale);

	for (int i = 0; i < 6; i++) {
		voxelTextures[i]->bind(i);
		voxelizeShader->setIndex("u_voxelTexture", i, i);
		voxelTextures[i]->clear(vec4(0,0,0,0));
		glBindImageTexture(i, voxelTexturea[i]->id, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
	}


	scene.draw(voxelizeShader);
	//render

	for (int i = 0; i < 6; i++) {
		voxelTextures[i]->bind(i);
		glGenerateMipmap(GL_TEXTURE_3D);
	}
}

void showVoxels() {

	visualizeShader->bind();
	visualizeShader->set("u_project", cam.final);
	visualizeShader->set("u_voxelScale", voxelScale);
	visualizeShader->set("u_voxelCount", voxelCount);
	visualizeShader->set("u_cameraPos", cam.pos);
	visualizeShader->set("u_voxelLod", voxelLod);


	for (int i = 0; i < 6; i++) {
		voxelTextures[i]->bind(i);
		visualizeShader->setIndex("u_voxelTexture", i, i);
	}

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glClear(GL_DEPTH_BUFFER_BIT);
	cubeMesh->draw();

}

void loadAssets() {

	flatShader = new Shader("flat.vert", "flat.frag");
	basicShader = new Shader("basic.vert", "basic.frag");
	voxelizeShader = new Shader("voxelize.vert", "voxelize.geom", "voxelize.frag");
	visualizeShader = new Shader("showvoxels.vert", "showvoxels.frag");
	voxelConeShader = new Shader("voxelcone.vert", "voxelcone.frag");
	shadowShader = new Shader("shadow.vert", "shadow.frag");

	bunnyMesh = loadMesh("../assets/models/bunny.obj");
	cubeMesh = loadMesh("../assets/models/cube.obj");
	planeMesh = loadMesh("../assets/models/plane.obj");
	sphereMesh = loadMesh("../assets/models/lp_sphere.obj");

	shadowMap = Framebuffer::shadowMap(2048, 2048);
}

void initScene() {

	scene.light = SpotLight(
		vec3(0, 0.99, 0),
		vec3(0, -1, 0),
		vec3(1,1,1),
		90.f, 2.f
		);

	scene.add(Object(
		"floor",
		planeMesh,
		Material(vec3(1)),
		vec3(0,-1,0),
		vec3(1,1,1),
		quat(vec3(0,0,0))
		));
	scene.add(Object(
		"floor",
		planeMesh,
		Material(vec3(1)),
		vec3(0,1,0),
		vec3(1,1,1),
		quat(vec3(pi,0,0))
		));
	scene.add(Object(
		"ceiling",
		planeMesh,
		Material(vec3(1)),
		vec3(0,0,-1),
		vec3(1,1,1),
		quat(vec3(pi*0.5,0,0))
		));

	scene.add(Object(
		"red",
		planeMesh,
		Material(vec3(1,0,0)),
		vec3(-1,0,0),
		vec3(1,1,1),
		quat(vec3(pi*0.5,pi*0.5,0))
		));
	scene.add(Object(
		"green",
		planeMesh,
		Material(vec3(0,1,0)),
		vec3(1,0,0),
		vec3(1,1,1),
		quat(vec3(pi*0.5,-pi*0.5,0))
		));

	scene.add(Object(
		"sphere",
		sphereMesh,
		Material(vec3(0,0,1)),
		vec3(3, 0, 0),
		vec3(1,1,1),
		quat(vec3(0,0,0))
		));

	scene.add(Object(
		"bunny",
		bunnyMesh, 
		Material(vec3(.7,.7,.7)), 
		vec3(.2,-1,.2), 
		vec3(.7,.7,.7),
		quat(vec3(0,0,0))
		));

	scene.add(Object(
		"cube",
		cubeMesh, 
		Material(vec3(.8,.8,.8)), 
		vec3(-.5,-.7,-.5), 
		vec3(.5,.2,.5),
		quat(vec3(0,.5,0))
		));

}

int main() {
	System::init(4, 6);

	Window::create(1280, 720, "title", Window::WINDOWED, false);

	Window::update();

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(MessageCallback, 0);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	int work_grp_cnt[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
	printf("max global (total) work group counts x:%i y:%i z:%i\n",
		work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

	int work_grp_size[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
	printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n",
		work_grp_size[0], work_grp_size[1], work_grp_size[2]);

	int work_grp_inv;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	printf("max local work group invocations %i\n", work_grp_inv);

	double oldTime = System::time();
	int updateCount = 0;

	loadAssets();
	initScene();

	initVoxelize();

	activeShader = voxelConeShader;


	cam = Camera(glm::radians(60.f), (float)Window::getWidth() / (float)Window::getHeight(), .1f, 100.f);

	cam.pos = vec3(0.f, 0.f, 3.f);

	bool showCursor = true;
	int lockPrev = false;
	Input::showCursor(showCursor);
	vec2 oldMousePos = Input::getMousePos();

	double prevTime = System::time();

	initChangeHandle();

	while(!Window::shouldClose()) {

		checkShaderChanges();

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
			// Window::setShouldClose(true);
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

		renderShadowMap();

		voxelize();

		Framebuffer::reset();
		glViewport(0, 0, winSize.x, winSize.y);
		glEnable(GL_BLEND);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		glEnable(GL_DEPTH_TEST);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthFunc(GL_LEQUAL);

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		activeShader->bind();
		activeShader->set("u_project", cam.final);
		activeShader->set("u_cameraPos", cam.pos);

		activeShader->set("u_voxelScale", voxelScale);
		activeShader->set("u_voxelCount", voxelCount);
		activeShader->set("u_offsetPos", offsetPos);
		activeShader->set("u_offsetDist", offsetDist);

		for (int i = 0; i < 6; i++) {
			voxelTextures[i]->bind(i);
			activeShader->setIndex("u_voxelTexture", i, i);
		}

		activeShader->set("u_lightProj", scene.light.getProjectionMatrix());
		activeShader->set("u_shadowBias", shadowBias);
		shadowMap->t->bind(6);
		activeShader->set("u_shadowMap", 6);

		scene.draw(activeShader);

		if (toggleVoxels) {
			showVoxels();
		}

		updateCount++;

		ImGui::Begin("Scene editor");

		if (ImGui::CollapsingHeader("Objects")) {
			for (u32 i = 0; i < scene.objects.size(); i++) {
				auto &obj = scene.objects[i];
				if (ImGui::TreeNode(obj.name.c_str())) {
					ImGui::DragFloat3("Pos", &obj.pos.x, 0.1f);
					ImGui::DragFloat3("Scale", &obj.scale.x, 0.1f);
					ImGui::TreePop();
				}
			}
		}

		if (ImGui::CollapsingHeader("Light")) {
			ImGui::DragFloat3("Pos", &scene.light.pos.x, 0.1f);
			ImGui::DragFloat3("Dir", &scene.light.dir.x, 0.1f);
			ImGui::DragFloat3("Color", &scene.light.color.x, 0.1f);
			ImGui::SliderFloat("Angle", &scene.light.angle, 0.f, 180.f);
			ImGui::SliderFloat("Smooth", &scene.light.smooth, 0.f, 180.f);
			ImGui::SliderFloat("Bias", &shadowBias, -0.00001f, 0.001f, "%.5f", 1.0f);
		}

		ImGui::End();

		ImGui::Begin("Renderer");
		ImGui::SliderFloat("LOD", &voxelLod, 0.f, 10.f);
		ImGui::Checkbox("show voxels", &toggleVoxels);
		ImGui::SliderFloat("Offset Pos", &offsetPos, -5.f, 5.f, "%.5f");
		ImGui::SliderFloat("Offset Dist", &offsetDist, 0.0f, 5.f, "%.5f");
		ImGui::End();

		Window::update();
	}

	System::exit(0);
	return 0;
}
