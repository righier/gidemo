#include <glad/glad.h>

#include "window.h"
#include "utils.h"
#include "camera.h"
#include "assets.h"
#include "scene.h"

#include "particlesystem.h"

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
Texture *voxelTexture;
Texture *voxelTextureAniso[6];

Shader *flatShader, *basicShader, *voxelizeShader, *visualizeShader, *voxelConeShader, *shadowShader;
Shader *mipmapShader, *fixopacityShader;

Shader *activeShader;

Mesh *cubeMesh;

float shadowBias = 0.0005f;
Framebuffer *shadowMap;

Camera cam;

bool enableIndirectLight = true;

char *renderModeText[] = {"Direct Light", "Indirect Diffuse", "Indirect Specular", "Global Illumination"};
int renderMode = 3;
int voxelCount = 128;

bool dynamicVoxelize = true;
bool customMipmap = false;
bool averageConflictingValues = false;
bool anisotropicVoxels = false;
string anisoHeader[] = {"", "#define ANISO\n"};
bool hdrVoxels = true;
bool temporalMultibounce = true;
bool addDiffuseNoise = true;

float multibounceRestitution = 0.4f;
float restitution = 0.4f;

bool enableTracedShadows = false;
float shadowAperture = 5;

bool toggleVoxels = false;
float voxelLod = 0;
int voxelIndex = 0;
float visualizeQuality = 10.0f;

bool depthPrepass = true;

bool lockfps = false;
bool vsyncStatus = false;
int targetfps = 5;



void initVoxelize() {
	if (anisotropicVoxels) {
		voxelTexture = Texture::init3D(voxelCount, false, hdrVoxels);
		for (int i = 0; i < 6; i++) {
			voxelTextureAniso[i] = Texture::init3D(voxelCount / 2, true, hdrVoxels);
		}
	} else {
		voxelTexture = Texture::init3D(voxelCount, true, hdrVoxels);
	}
}

void disposeVoxels(bool flip = false) {
	voxelTexture->dispose();
	delete voxelTexture;

	if (anisotropicVoxels ^ flip) {
		for (int i = 0; i < 6; i++) {
			voxelTextureAniso[i]->dispose();
			delete voxelTextureAniso[i];
		}
	}
}

void renderShadowMap(double time) {

	shadowShader->bind();
	shadowShader->set("u_project", scene.light.getProjectionMatrix());

	shadowMap->bind();
	shadowMap->setViewport();

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);

	scene.draw(time, shadowShader, false);

}

void voxelize(double time) {

	if (!dynamicVoxelize) return;

	voxelizeShader->bind();


	Framebuffer::reset();
	glViewport(0, 0, voxelCount, voxelCount);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	// glEnable(GL_MULTISAMPLE);
 
	voxelTexture->bind(0);
	voxelTexture->clear(vec4(0,0,0,0));
	voxelizeShader->set("u_voxelTexture", 0);
	glBindImageTexture(0, voxelTexture->id, 0, GL_TRUE, 0, GL_READ_WRITE, voxelTexture->storageType);

	voxelizeShader->set("u_lightProj", scene.light.getProjectionMatrix());
	voxelizeShader->set("u_shadowBias", shadowBias);

 	shadowMap->t->bind(1);
	voxelizeShader->set("u_shadowMap", 1);
	voxelizeShader->set("u_averageValues", averageConflictingValues && !hdrVoxels);
	voxelizeShader->set("u_temporalMultibounce", temporalMultibounce);
	voxelizeShader->set("u_aniso", anisotropicVoxels);
	voxelizeShader->set("u_voxelCount", voxelCount);
	voxelizeShader->set("u_time", (float)time);
	voxelizeShader->set("u_restitution", multibounceRestitution);

	if (!anisotropicVoxels) {
		voxelTexture->bind(8);
		voxelizeShader->set("u_voxelTexturePrev", 8);
	} else {
		for (int i = 0; i < 6; i++) {
			voxelTextureAniso[i]->bind(i+2);
			voxelizeShader->setIndex("u_voxelTexturePrev", i, i+2);
		}
	}


	scene.draw(time, voxelizeShader);
	//render

	// if (averageConflictingValues && !hdrVoxels) {
		fixopacityShader->bind();
		fixopacityShader->set("u_voxel", 0);
		glBindImageTexture(0, voxelTexture->id, 0, GL_TRUE, 0, GL_READ_WRITE, voxelTexture->storageType);
		fixopacityShader->dispatch(voxelCount, voxelCount, voxelCount);
	// }

	if (anisotropicVoxels) {
		for (int dir = 0; dir < 6; dir++) {
			genMipmapLevel(mipmapShader, voxelTexture, voxelTextureAniso[dir], 0, voxelCount/2, true, dir);
			genMipmap(mipmapShader, voxelTextureAniso[dir], voxelCount/4, true, dir);
		}
	} else if (customMipmap) {
		genMipmap(mipmapShader, voxelTexture, voxelCount/2);
	} else {
		voxelTexture->bind(0);
		glGenerateMipmap(GL_TEXTURE_3D);

	}
}

void showVoxels() {

	visualizeShader->bind();
	visualizeShader->set("u_project", cam.final);
	visualizeShader->set("u_voxelCount", voxelCount);
	visualizeShader->set("u_cameraPos", cam.pos);
	visualizeShader->set("u_voxelLod", voxelLod);
	visualizeShader->set("u_quality", visualizeQuality);
	visualizeShader->set("u_aniso", anisotropicVoxels);
	visualizeShader->set("u_voxelIndex", voxelIndex);

	voxelTexture->bind(0);
	visualizeShader->set("u_voxelTexture", 0);

	for (int i = 0; i < 6; i++) {
		if (anisotropicVoxels) {
			voxelTextureAniso[i]->bind(i+1);
		}
		visualizeShader->setIndex("u_voxelTextureAniso", i, i+1);
	}

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glClear(GL_DEPTH_BUFFER_BIT);
	cubeMesh->draw();

}

void recompileShaders() {
	voxelizeShader->header = anisoHeader[anisotropicVoxels];
	visualizeShader->header = anisoHeader[anisotropicVoxels];
	voxelConeShader->header = anisoHeader[anisotropicVoxels];
	mipmapShader->header = anisoHeader[anisotropicVoxels];
	voxelizeShader->load();
	visualizeShader->load();
	voxelConeShader->load();
	mipmapShader->load();
}

void loadAssets() {

	flatShader = assets.add("flat_shader", new Shader("flat.vert", "flat.frag", ""));
	basicShader = assets.add("basic_shader", new Shader("basic.vert", "basic.frag", ""));
	voxelizeShader = assets.add("voxelize_shader", new Shader("voxelize.vert", "voxelize.geom", "voxelize.frag", anisoHeader[anisotropicVoxels]));
	visualizeShader = assets.add("visualize_shader", new Shader("showvoxels.vert", "showvoxels.frag", anisoHeader[anisotropicVoxels]));
	voxelConeShader = assets.add("voxel_cone_shader", new Shader("voxelcone.vert", "voxelcone.frag", anisoHeader[anisotropicVoxels]));
	shadowShader = assets.add("shadow_shader", new Shader("shadow.vert", "shadow.frag", ""));

	mipmapShader = assets.add("mipmap_shader", new Shader("mipmap.comp", anisoHeader[anisotropicVoxels]));
	fixopacityShader = assets.add("fixopacity_shader", new Shader("fixopacity.comp", ""));

	cubeMesh = loadMesh("../assets/models/cube.obj");

	shadowMap = Framebuffer::shadowMap(2048, 2048);
}

void addCornellBox(bool useTexture = false) {
	auto *planeMesh = loadMesh("../assets/models/plane.obj");

	scene.light = SpotLight(
		vec3(0, 1, 0),
		vec3(0, -1, 0),
		useTexture ? vec3(0) : vec3(3),
		110.f, .5f
		);

	if(useTexture) {
		scene.add(Object(
			"floor",
			planeMesh,
			Material(
				loadTexture("../assets/suntemple/textures/M_FloorTiles2_Inst_0_BaseColor.jpg"),
				loadTexture("../assets/suntemple/textures/M_FloorTiles2_Inst_0_Normal.jpg"),
				loadTexture("../assets/suntemple/textures/M_FloorTiles2_Inst_0_Specular.jpg"),
				blackTexture()
				),
			vec3(0,-1,0),
			vec3(1,1,1),
			quat(vec3(0,0,0))
			));
	} else {
		scene.add(Object(
			"floor",
			planeMesh,
			Material(vec3(1)),
			vec3(0,-1,0),
			vec3(1,1,1),
			quat(vec3(0,0,0))
			));
	}
	scene.add(Object(
		"ceiling",
		planeMesh,
		Material(vec3(1)),
		vec3(0,1,0),
		vec3(1,1,1),
		quat(vec3(pi,0,0))
		));

	if (useTexture) {
		Texture *painting = loadTexture("../assets/textures/painting.jpg");
		scene.add(Object(
			"painting",
			planeMesh,
			Material(painting, normalTexture(), whiteTexture(), painting),
			vec3(0,0,-1),
			vec3(1,1,1),
			quat(vec3(pi*0.5,0,0))
			));
	} else {
		scene.add(Object(
			"wall",
			planeMesh,
			Material(vec3(1)),
			vec3(0,0,-1),
			vec3(1,1,1),
			quat(vec3(pi*0.5,0,0))
			));
	}
	// scene.add(Object(
	// 	"frontWall",
	// 	planeMesh,
	// 	Material(vec3(1)),
	// 	vec3(0,0,1),
	// 	vec3(1,1,1),
	// 	quat(vec3(-pi*0.5,0,0))
	// 	));

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
}

void bunnyScene() {
	auto *bunnyMesh = loadMesh("../assets/models/bunny.obj");

	addCornellBox();

	scene.add(Object(
		"bunny",
		bunnyMesh, 
		Material(vec3(0.8,0.6,0.0),vec3(),1,0.2f), 
		vec3(.2,-1.,.2), 
		vec3(.7,.7,.7),
		quat(vec3(0,0,0))
		));	
}

void horseScene(bool useTexture = false) {
	auto *horseMesh = loadMesh("../assets/models/horse.obj");

	addCornellBox(useTexture);

	scene.add(Object(
		"horse",
		horseMesh, 
		Material(vec3(1)), 
		vec3(0,-1.,0), 
		vec3(1,1,1),
		quat(vec3(0,glm::radians(45.f),0))
		));
}

void benchScene() {
	auto *sphereMesh = loadMesh("../assets/models/sphere.obj");

	addCornellBox();

	scene.add(Object(
		"big",
		sphereMesh,
		Material(vec3(1), vec3(0), 0, 0.5),
		vec3(-0.45, -0.45, -0.27371),
		vec3(0.55, 0.55, 0.55),
		quat(vec3(0,0,0))
		));

	scene.add(Object(
		"small",
		sphereMesh,
		Material(vec3(1), vec3(0), 0, 0.5),
		vec3(0.52374, -0.725, 0.28117),
		vec3(0.275, 0.275, 0.275),
		quat(vec3(0,0,0))
		));
}

void particleScene() {
	scene.add(ParticleSystem());
}

void pbrScene(bool useTexture = false) {
	auto *sphereMesh = loadMesh("../assets/models/sphere.obj");

	addCornellBox(useTexture);

	for (int i = 0; i <= 10; i++) {
		for (int j = 0; j <= 1; j++) {
			float rough = (float)i/10.f;
			float metal = (float)j/1.f;

			float scale = 0.07f;
			float ps = 0.8f;
			float x = i*.2f*ps - ps;
			float z = 0.0;
			float y = j*.5f - 0.5f;

			scene.add(Object(
				"sphere" + std::to_string(i) + "_" + std::to_string(j),
				sphereMesh,
				Material(vec3(1), vec3(0), metal, rough),
				vec3(x, y, z),
				vec3(scale, scale, scale),
				quat(vec3(0,0,0))
				));

		}
	}

}


void templeScene() {
	scene.light = SpotLight(
		vec3(0, 0.2, -0.5),
		vec3(0, -1, 1),
		vec3(10),
		70.f, 20.f
		);	

	shadowBias = 0.00006f;

	loadScene(scene, "../assets/suntemple/suntemple.obj");

	auto statueBase = scene.get("BottomTrim_Inst_Black");
	statueBase->mat.rough = 0.3f;

	auto floor1 = scene.get("FloorTiles1_Inst_Inst2");
	auto floor2 = scene.get("FloorTiles2_Inst");
	auto floor3 = scene.get("FloorTiles2_Inst_Inst_Inst");
	floor1->mat.rough = 0.3f;
	floor2->mat.rough = 0.3f;
	floor3->mat.rough = 0.3f;

	auto fire = scene.get("FirePit_Inst_Glow");
	fire->mat.emissionScale = 1000.f;
}

int main() {
	System::init(4, 6);


	Window::create(1280, 720, "title", Window::WINDOWED, vsyncStatus);

	Window::update();

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(MessageCallback, 0);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

	int maxTextures;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextures);
	LOG("max texture units:", maxTextures);

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


	// templeScene();
	// horseScene(false);
	particleScene();
	loadAssets();

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

		double targetDelta = 1.0 / (double)targetfps;
		if (lockfps && timeDelta < targetDelta) {
			double sleepTime = targetDelta - timeDelta - 0.001;
			if (sleepTime > 0) {
				System::sleep(targetDelta - timeDelta);
			}
			continue;
		}

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

		float moveSpeed = 0.5f;
		move = move * moveSpeed * (float)timeDelta;
		move = glm::rotateY(move, cam.hrot);

		cam.ratio = winSize.x / winSize.y;
		cam.rotate(mouseDelta.x, mouseDelta.y);
		cam.move(move);
		cam.update();

		renderShadowMap(time);

		voxelize(time);

		Framebuffer::reset();
		glViewport(0, 0, (int)winSize.x, (int)winSize.y);

		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (depthPrepass && !toggleVoxels) {
			// glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			// glDepthFunc(GL_LESS);

			shadowShader->bind();
			Framebuffer::reset();
			shadowShader->set("u_project", cam.final);

			scene.draw(time, shadowShader, true);

			// glDepthFunc(GL_EQUAL);
			// glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}

		// glDisable(GL_BLEND);
		// glEnable(GL_CULL_FACE);
		// glCullFace(GL_BACK);


		if (!toggleVoxels) {


			activeShader->bind();
			Framebuffer::reset();
			activeShader->set("u_project", cam.final);
			activeShader->set("u_cameraPos", cam.pos);

			activeShader->set("u_tracedShadows", enableTracedShadows);
			activeShader->set("u_shadowAperture", shadowAperture);

			activeShader->set("u_renderMode", renderMode);
			activeShader->set("u_indirectLight", enableIndirectLight);
			activeShader->set("u_voxelCount", voxelCount);
			activeShader->set("u_diffuseNoise", addDiffuseNoise);
			activeShader->set("u_restitution", restitution);

			voxelTexture->bind(1);
			activeShader->set("u_voxelTexture", 1);
			activeShader->set("u_aniso", anisotropicVoxels);
			for (int i = 0; i < 6; i++) {
				if (anisotropicVoxels) {
					voxelTextureAniso[i]->bind(i+2);
				}
				activeShader->setIndex("u_voxelTextureAniso", i, i+2);
			}

			activeShader->set("u_lightProj", scene.light.getProjectionMatrix());
			activeShader->set("u_shadowBias", shadowBias);
			shadowMap->t->bind(0);
			activeShader->set("u_shadowMap", 0);

			activeShader->set("u_time", (float)time);

			scene.draw(time, activeShader);

		} else {
			showVoxels();
		}

		updateCount++;

		if (showCursor) {

			ImGui::ShowDemoWindow();
			ImGui::Begin("Scene editor");

			if (ImGui::CollapsingHeader("Objects")) {
				for (u32 i = 0; i < scene.objects.size(); i++) {
					auto &obj = scene.objects[i];
					if (ImGui::TreeNode(obj.name.c_str())) {
						ImGui::DragFloat3("Pos", &obj.pos.x, 0.05f);
						ImGui::DragFloat3("Scale", &obj.scale.x, 0.05f);
						ImGui::ColorEdit3("Diffuse", (float *)&obj.mat.diffuse);
						ImGui::ColorEdit3("Emission", (float *)&obj.mat.emission);
						ImGui::DragFloat("Emission Scale", &obj.mat.emissionScale, 0.1f);
						ImGui::SliderFloat("Roughness", &obj.mat.rough, 0.f, 1.f);
						ImGui::SliderFloat("Metalness", &obj.mat.metal, 0.f, 1.f);
						ImGui::TreePop();
					}
				}
			}

			if (ImGui::CollapsingHeader("Particle Systems")) {

				auto uiPropFloat = [](const string &name, Prop<float> &p) {

				};

				auto uiPropVec3 = [](const string &name, Prop<vec3> &p) {

				};

				auto uiPropColor = [](const string &name, Prop<vec3> &p) {

				};

				auto uiAnimFloat = [](const string &name, AnimProp<float> &p) {

				};

				auto uiAnimVec3 = [](const string &name, AnimProp<vec3> &p) {

				};

				auto uiAnimColor = [](const string &name, AnimProp<vec3> &p) {

				};

				for (u32 i = 0; i < scene.particles.size(); i++) {
					auto &ps = scene.particles[i];
					if (ImGui::TreeNode(std::to_string(i).c_str())) {
						ImGui::DragInt("Particle Count", (int *)&(ps.count));
						uiPropFloat("Particle Life Time", ps.life);
						uiPropVec3("Particle Position", ps.pos);
						uiPropVec3("Particle Velocity", ps.vel);
						uiPropVec3("Particle Acceleration", ps.acc);
						uiAnimVec3("Particle Scale", ps.scale);
						uiAnimColor("Particle Color", ps.color);
						uiAnimFloat("Particle Opacity", ps.opacity);
						uiPropFloat("Particle Fadein Time", ps.fadeIn);

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
			if (ImGui::Checkbox("vsync", &vsyncStatus)) {
				Window::setVSyncStatus(vsyncStatus);
			}

			ImGui::SliderInt("FPS target", &targetfps, 1, 120);

			int prevVoxelCount = voxelCount;
			const char* voxelCounts[] = {"16", "32", "64", "128", "256", "512", "1024"};

			int currVolumeItem = (int)round(log2(prevVoxelCount/16));

			ImGui::Combo("Volume Size", &currVolumeItem, voxelCounts, 7);

			voxelCount = (1 << currVolumeItem) * 16;

			if (voxelCount != prevVoxelCount) {
				LOG("changed voxel count");
				disposeVoxels();
				initVoxelize();
			}

			ImGui::Checkbox("Dynamic Voxelization", &dynamicVoxelize);
			ImGui::Checkbox("Average Conflicting Values", &averageConflictingValues);
			ImGui::Checkbox("Custom mipmap", &customMipmap);
			if (ImGui::Checkbox("Anisotropic Voxels", &anisotropicVoxels)) {
				disposeVoxels(true);
				initVoxelize();
				recompileShaders();
			}
			if (ImGui::Checkbox("HDR Voxels", &hdrVoxels)) {
				disposeVoxels();
				initVoxelize();
			}
			ImGui::Checkbox("Temporal Multibounce", &temporalMultibounce);
			if (temporalMultibounce) {
				ImGui::SliderFloat("Multibounce restitution", &multibounceRestitution, 0, 1, "%.3f");
			}
			ImGui::Checkbox("Show Voxels", &toggleVoxels);
			if (toggleVoxels) {
				ImGui::SliderFloat("Quality", &visualizeQuality, 1, 50);
				ImGui::SliderFloat("LOD", &voxelLod, 0.f, log2f((float)voxelCount+1.f), "%.0f");
				if (anisotropicVoxels) {
					ImGui::SliderInt("Dir", &voxelIndex, 0, 5);
				}
			} else {
				ImGui::Checkbox("Depth Prepass", &depthPrepass);
				ImGui::Combo("Render Mode", &renderMode, renderModeText, 4);
				ImGui::Checkbox("Add Diffuse Noise", &addDiffuseNoise);
				ImGui::Checkbox("Enable Traced Shadows", &enableTracedShadows);
				if (enableTracedShadows) {
					ImGui::SliderFloat("Traced Shadow Aperture", &shadowAperture, 0.1f, 90.f, "%.01f");
				}
			}
			ImGui::SliderFloat("Diffuse restitution", &restitution, 0, 1, "%.3f");
			ImGui::End();	
		}


		Window::update();
	}

	System::exit(0);
	return 0;
}
