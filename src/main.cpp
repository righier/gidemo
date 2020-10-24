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
Texture *voxelTexture;
Texture *voxelTextureAniso[6];

Shader *flatShader, *basicShader, *voxelizeShader, *visualizeShader, *voxelConeShader, *shadowShader;
Shader *mipmapShader, *fixopacityShader;
Mesh *bunnyMesh, *horseMesh, *cubeMesh, *planeMesh, *sphereMesh, *lpsphereMesh;

Shader *activeShader;

float shadowBias = 0.00007f;
Framebuffer *shadowMap;

Camera cam;

int voxelCount = 128;
float offsetPos = 0.0f;
float offsetDist = 0.25f;

bool dynamicVoxelize = true;
bool customMipmap = true;
bool averageConflictingValues = true;
bool anisotropicVoxels = true;
bool temporalMultibounce = false;
bool addDiffuseNoise = false;

bool toggleVoxels = false;
float voxelLod = 0;
int voxelIndex = 0;
float visualizeQuality = 10.0f;

bool lockfps = false;
bool vsyncStatus = false;
int targetfps = 5;

void initVoxelize() {
	if (anisotropicVoxels) {
		voxelTexture = Texture::init3D(voxelCount, false);
		for (int i = 0; i < 6; i++) {
			voxelTextureAniso[i] = Texture::init3D(voxelCount / 2, true);
		}
	} else {
		voxelTexture = Texture::init3D(voxelCount, true);
	}
}

void disposeVoxels() {
	voxelTexture->dispose();
	delete voxelTexture;

	if (anisotropicVoxels) {
		for (int i = 0; i < 6; i++) {
			voxelTextureAniso[i]->dispose();
			delete voxelTextureAniso[i];
		}
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
	glBindImageTexture(0, voxelTexture->id, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);

	voxelizeShader->set("u_lightProj", scene.light.getProjectionMatrix());
	voxelizeShader->set("u_shadowBias", shadowBias);

 	shadowMap->t->bind(1);
	voxelizeShader->set("u_shadowMap", 1);
	voxelizeShader->set("u_averageValues", averageConflictingValues);
	voxelizeShader->set("u_temporalMultibounce", temporalMultibounce);
	voxelizeShader->set("u_aniso", anisotropicVoxels);
	voxelizeShader->set("u_voxelCount", voxelCount);

	if (temporalMultibounce) {
		if (anisotropicVoxels) {
			for (int i = 0; i < 6; i++) {
				voxelTextureAniso[i]->bind(i+2);
				voxelizeShader->setIndex("u_voxelTextureAniso", i, i+2);
			}
		} else {
			voxelizeShader->set("u_voxelTexturePrev", 2);
			voxelTexture->bind(2);
		}
	}

	scene.draw(voxelizeShader);
	//render

	if (averageConflictingValues) {
		fixopacityShader->bind();
		fixopacityShader->set("u_voxel", 0);
		glBindImageTexture(0, voxelTexture->id, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
		fixopacityShader->dispatch(voxelCount, voxelCount, voxelCount);
	}

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

	if (anisotropicVoxels) {
		for (int i = 0; i < 6; i++) {
			voxelTextureAniso[i]->bind(i+1);
			visualizeShader->setIndex("u_voxelTextureAniso", i, i+1);
		}
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
	fixopacityShader = assets.add("fixopacity_shader", new Shader("fixopacity.comp"));

	bunnyMesh = loadMesh("../assets/models/bunny.obj");
	horseMesh = loadMesh("../assets/models/horse.obj");
	cubeMesh = loadMesh("../assets/models/cube.obj");
	planeMesh = loadMesh("../assets/models/plane.obj");
	lpsphereMesh = loadMesh("../assets/models/lp_sphere.obj");
	sphereMesh = loadMesh("../assets/models/sphere.obj");

	shadowMap = Framebuffer::shadowMap(2048, 2048);
}

void addCornellBox() {
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
	addCornellBox();

	scene.add(Object(
		"bunny",
		bunnyMesh, 
		Material(vec3(1)), 
		vec3(.2,-1.,.2), 
		vec3(.7,.7,.7),
		quat(vec3(0,0,0))
		));	
}

void horseScene() {
	addCornellBox();

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

void pbrScene() {
	addCornellBox();

	for (int i = 0; i <= 10; i++) {
		for (int j = 0; j <= 10; j++) {
			float rough = (float)i/10.f;
			float metal = (float)j/10.f;

			float scale = 0.07f;
			float ps = 0.8f;
			float x = i*.2f*ps - ps;
			float z = j*.2f*ps - ps;
			float y = -0.8f;

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

	loadAssets();
	benchScene();

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

			activeShader->set("u_voxelCount", voxelCount);
			activeShader->set("u_offsetPos", offsetPos);
			activeShader->set("u_offsetDist", offsetDist);
			activeShader->set("u_diffuseNoise", addDiffuseNoise);

			voxelTexture->bind(1);
			activeShader->set("u_voxelTexture", 1);
			activeShader->set("u_aniso", anisotropicVoxels);
			if (anisotropicVoxels) {
				for (int i = 0; i < 6; i++) {
					voxelTextureAniso[i]->bind(i+2);
					activeShader->setIndex("u_voxelTextureAniso", i, i+2);
				}
			}

			activeShader->set("u_lightProj", scene.light.getProjectionMatrix());
			activeShader->set("u_shadowBias", shadowBias);
			shadowMap->t->bind(0);
			activeShader->set("u_shadowMap", 0);

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
			disposeVoxels();
			initVoxelize();
		}
		ImGui::Checkbox("Temporal Multibounce", &temporalMultibounce);
		ImGui::Checkbox("Add Diffuse Noise", &addDiffuseNoise);
		ImGui::Checkbox("Show Voxels", &toggleVoxels);
		if (toggleVoxels) {
			ImGui::SliderFloat("Quality", &visualizeQuality, 1, 50);
			ImGui::SliderFloat("LOD", &voxelLod, 0.f, log2f((float)voxelCount)+1.f, "%.0f");
			if (anisotropicVoxels) {
				ImGui::SliderInt("Dir", &voxelIndex, 0, 5);
			}
		} else {
			ImGui::SliderFloat("Offset Pos", &offsetPos, -5.f, 5.f, "%.5f");
			ImGui::SliderFloat("Offset Dist", &offsetDist, 0.0f, 5.f, "%.5f");
		}
		ImGui::End();

		Window::update();
	}

	System::exit(0);
	return 0;
}
