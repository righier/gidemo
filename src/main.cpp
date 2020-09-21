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

	if (type == 0x8251) return;
	
	fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

Scene scene;
AssetStore assets;

// Texture *voxelTexture;
Texture *voxelTextures[6];

Shader *flatShader, *basicShader, *voxelizeShader, *visualizeShader, *voxelConeShader, *shadowShader;
Shader *mipmapShader;
Mesh *bunnyMesh, *cubeMesh, *planeMesh, *sphereMesh;

Shader *activeShader;

float shadowBias = 0.00007f;
Framebuffer *shadowMap;

Camera cam;

int voxelCount = 128;
float voxelScale = 1.01f;
float offsetPos = 0.0f;
float offsetDist = 0.25f;

bool dynamidVoxelize = true;
bool toggleVoxels = false;
bool customMipmap = true;
float voxelLod = 0;
int voxelIndex = 0;

bool lockfps = false;
int targetfps = 5;

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

	if (!dynamidVoxelize) return;

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
		glBindImageTexture(i, voxelTextures[i]->id, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
	}


	scene.draw(voxelizeShader);
	//render

	if (customMipmap) {
		for (int i = 0; i < 6; i++) {
			mipmapShader->bind();

			u32 size = voxelCount / 2;
			u32 level = 1;
			while (size > 0) {
				mipmapShader->set("u_level", std::max(0.002f, (float)(level-1)));
				mipmapShader->set("u_dir", i);
				mipmapShader->set("u_src", 0);
				voxelTextures[i]->bind(0);
				mipmapShader->set("u_dest", 1);
				glBindImageTexture(1, voxelTextures[i]->id, level, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);

				mipmapShader->dispatch(size, size, size);
				size /= 2;
				level++;
			}
		}
	} else {
		for (int i = 0; i < 6; i++) {
			voxelTextures[i]->bind(i);
			glGenerateMipmap(GL_TEXTURE_3D);
		}
	}
}

void showVoxels() {

	visualizeShader->bind();
	visualizeShader->set("u_project", cam.final);
	visualizeShader->set("u_voxelScale", voxelScale);
	visualizeShader->set("u_voxelCount", voxelCount);
	visualizeShader->set("u_cameraPos", cam.pos);
	visualizeShader->set("u_voxelLod", voxelLod);
	visualizeShader->set("u_voxelIndex", voxelIndex);


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

	flatShader = assets.add("flat_shader", new Shader("flat.vert", "flat.frag"));
	basicShader = assets.add("basic_shader", new Shader("basic.vert", "basic.frag"));
	voxelizeShader = assets.add("voxelize_shader", new Shader("voxelize.vert", "voxelize.geom", "voxelize.frag"));
	visualizeShader = assets.add("visualize_shader", new Shader("showvoxels.vert", "showvoxels.frag"));
	voxelConeShader = assets.add("voxel_cone_shader", new Shader("voxelcone.vert", "voxelcone.frag"));
	shadowShader = assets.add("shadow_shader", new Shader("shadow.vert", "shadow.frag"));

	mipmapShader = assets.add("mipmap_shader", new Shader("mipmap.comp"));

	bunnyMesh = loadMesh("../assets/models/bunny.obj");
	cubeMesh = loadMesh("../assets/models/cube.obj");
	planeMesh = loadMesh("../assets/models/plane.obj");
	sphereMesh = loadMesh("../assets/models/lp_sphere.obj");

	shadowMap = Framebuffer::shadowMap(2048, 2048);
}

void initScene() {

	scene.light = SpotLight(
		vec3(0, 0.8, 0),
		vec3(0, -1, 0),
		vec3(1,1,1),
		110.f, 2.f
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
		"ceiling",
		planeMesh,
		Material(vec3(1)),
		vec3(0,1,0),
		vec3(1,1,1),
		quat(vec3(pi,0,0))
		));
	scene.add(Object(
		"wall",
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
		Material(vec3(1)), 
		vec3(.2,-1,.2), 
		vec3(.7,.7,.7),
		quat(vec3(0,0,0))
		));

	scene.add(Object(
		"cube",
		cubeMesh, 
		Material(vec3(1)), 
		vec3(-.5,-.7,-.5), 
		vec3(.5,.2,.5),
		quat(vec3(0,.5,0))
		));

}

int main() {
	System::init(4, 6);

	Window::create(1280, 720, "title", Window::WINDOWED, true);

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

	while(!Window::shouldClose()) {
		assets.update();

		double time = System:: time();
		float timeDelta = (float)(time - prevTime);
		prevTime = time;

    double targetDelta = 1.0 / (double)targetfps;
    if (lockfps && timeDelta < targetDelta) {
      System::sleep(targetDelta - timeDelta);
    }

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

		renderShadowMap();

		voxelize();

		Framebuffer::reset();
		glViewport(0, 0, (int)winSize.x, (int)winSize.y);
		glEnable(GL_BLEND);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		glEnable(GL_DEPTH_TEST);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthFunc(GL_LEQUAL);

		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (!toggleVoxels) {


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

		} else {
			showVoxels();
		}

		updateCount++;

		ImGui::Begin("Scene editor");

		if (ImGui::CollapsingHeader("Objects")) {
			for (u32 i = 0; i < scene.objects.size(); i++) {
				auto &obj = scene.objects[i];
				if (ImGui::TreeNode(obj.name.c_str())) {
					ImGui::DragFloat3("Pos", &obj.pos.x, 0.05f);
					ImGui::DragFloat3("Scale", &obj.scale.x, 0.05f);
					ImGui::ColorEdit3("Diffuse", (float *)&obj.mat.diffuse);
					ImGui::ColorEdit3("Emission", (float *)&obj.mat.emission);
					ImGui::SliderFloat("Roughness", &obj.mat.rough, 0.f, 1.f);
					ImGui::SliderFloat("Metalness", &obj.mat.metal, 0.f, 1.f);
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
    ImGui::Checkbox("lock FPS", &lockfps);
    ImGui::SliderInt("FPS target", &targetfps, 1, 120);
		ImGui::Checkbox("voxelize", &dynamidVoxelize);
		ImGui::Checkbox("custom mipmap", &customMipmap);
		ImGui::SliderFloat("LOD", &voxelLod, 0.f, 7.f, "%.0f");
		ImGui::SliderInt("Dir", &voxelIndex, 0, 5);
		ImGui::Checkbox("show voxels", &toggleVoxels);
		ImGui::SliderFloat("Offset Pos", &offsetPos, -5.f, 5.f, "%.5f");
		ImGui::SliderFloat("Offset Dist", &offsetDist, 0.0f, 5.f, "%.5f");
		ImGui::End();

		Window::update();
	}

	System::exit(0);
	return 0;
}
