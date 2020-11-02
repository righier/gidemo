#include "scene.h"

#include "utils.h"
#include "object.h"
#include "assets.h"


#include <unordered_map>
#include "fast_obj.h"

void loadScene(Scene &scene, const char *path) {
	LOG("loading scene:", path);

	fastObjMesh *obj = fast_obj_read(path);	

	LOG(obj->position_count, obj->texcoord_count, obj->normal_count);

	LOG(obj->group_count);

	LOG("loading materials");
	vector<Material> materials;

	for (size_t i = 0; i < obj->material_count; i++) {
		fastObjMaterial &mat = obj->materials[i];
		LOG("material:", mat.name);

		Texture *dMap = loadTexture(mat.map_Kd.path);
		Texture *bMap = loadTexture(mat.map_bump.path);
		Texture *sMap = loadTexture(mat.map_Ks.path);
		Texture *eMap = loadTexture(mat.map_Ke.path);
		if (dMap == nullptr) dMap = whiteTexture();
		if (bMap == nullptr) bMap = normalTexture();
		if (sMap == nullptr) sMap = whiteTexture();
		if (eMap == nullptr) eMap = blackTexture();

		Material m(dMap, bMap, sMap, eMap);
		materials.push_back(m);
	}

	for (size_t i = 0; i < obj->group_count; i++) {
		fastObjGroup &group = obj->groups[i];

		string name = group.name;
		Mesh *mesh = loadMesh(obj, (int)i);
		Material mat = materials[obj->face_materials[group.face_offset]];

		scene.add(Object(
			name,
			mesh,
			mat,
			vec3(0),
			vec3(1),
			quat(vec3(0))
			));

	}

	fast_obj_destroy(obj);
}