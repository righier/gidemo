
#include <glad/glad.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <unordered_map>

#include "assets.h"

Mesh::Mesh(vector<Vertex> &vertices, vector<u32> &indices):
vertices(std::move(vertices)),
indices(std::move(indices)) {
	setup();
}

Mesh::Mesh(Mesh &&other):
vertices(std::move(other.vertices)),
indices(std::move(other.indices)),
vao(other.vao),
vbo(other.vbo),
ebo(other.ebo) {
	other.vao = 0;
}

Mesh &Mesh::operator=(Mesh &&other) {
	if (this != &other) {
		dispose();
		vertices = std::move(other.vertices);
		indices = std::move(other.indices);
		vao = other.vao;
		vbo = other.vbo;
		ebo = other.ebo;

		other.vao = 0;
	}

	return *this;
}


Mesh::~Mesh() {
	dispose();
}

/* loads mesh to GPU memory */
void Mesh::setup() {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(u32), &indices[0], GL_STATIC_DRAW);

	/* position, uv, normal, tangent, bitangent */
	glEnableVertexAttribArray(0);	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
	glEnableVertexAttribArray(1);	
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(2);	
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));
	glEnableVertexAttribArray(3);	
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, t));
	glEnableVertexAttribArray(4);	
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, b));

	glBindVertexArray(0);

	LOG("CREATED MESH WITH N VERTICES:", vertices.size());
	LOG("CREATED MESH WITH N FACES:", indices.size() / 3);
}

void Mesh::bind() {
	glBindVertexArray(vao);
}

void Mesh::reset() {
	glBindVertexArray(0);
}

void Mesh::drawNoBind() {
	glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
}

void Mesh::draw() {
	Mesh::bind();
	Mesh::drawNoBind();
	Mesh::reset();
}

void Mesh::dispose() {
	if (vao) {
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteBuffers(1, &ebo);
	}
}

bool operator==(const fastObjIndex &a, const fastObjIndex &b) {
	return a.p == b.p && a.t == b.t && a.n == b.n;
}

Mesh *loadMesh(void *_obj, int group_id) {
	fastObjMesh *obj = (fastObjMesh *)_obj;

	vector<Vertex> vertices;
	vector<u32> indices;

	auto fun = [](const fastObjIndex &val) {
		return hash(&val, sizeof(val));
	};

	/* map to reuse duplicate vertices */
	std::unordered_map<fastObjIndex, u32, decltype(fun)> found(10, fun);

	auto &group = obj->groups[group_id];
	u32 start = group.index_offset;
	u32 end = start + group.face_count * 3;

	// generate unique vertices and indices
	for (u32 i = start; i < end; i++) {
		auto &index = obj->indices[i];

		if (found.count(index) == 0) {
			Vertex v;
			v.p = glm::make_vec3(obj->positions + index.p * 3);
			v.uv = glm::make_vec2(obj->texcoords + index.t * 2);
			v.n = glm::make_vec3(obj->normals + index.n * 3);
			v.t = vec3(0);
			v.b = vec3(0);

			found[index] = (u32)vertices.size();
			vertices.push_back(v);
		}

		indices.push_back(found[index]);
	}

	// generate tangent and bitangent vectors
	for (u32 i = 0; i < indices.size(); i += 3) {
		Vertex &v0 = vertices[indices[i]];
		Vertex &v1 = vertices[indices[i+1]];
		Vertex &v2 = vertices[indices[i+2]];

		/* face edges in pos and uv coordinates */
		vec3 dp1 = v1.p - v0.p;
		vec3 dp2 = v2.p - v0.p;
		vec2 dt1 = v1.uv - v0.uv;
		vec2 dt2 = v2.uv - v0.uv;

		vec3 tan, bit;
		tan.x = dt2.y * dp1.x - dt1.y * dp2.x;
		tan.y = dt2.y * dp1.y - dt1.y * dp2.y;
		tan.z = dt2.y * dp1.z - dt1.y * dp2.z;
		bit.x = dt2.x * dp1.x - dt1.x * dp2.x;
		bit.y = dt2.x * dp1.y - dt1.x * dp2.y;
		bit.z = dt2.x * dp1.z - dt1.x * dp2.z;
		float f = 1.0f / (dt1.x * dt2.y - dt1.y * dt2.x);
		tan = glm::normalize(f * tan);
		bit = glm::normalize(f * bit);

		v0.t += tan;
		v0.b += bit;
		v1.t += tan;
		v1.b += bit;
		v2.t += tan;
		v2.b += bit;
	}

	// normalize tangent and bitangent
	for (Vertex &v: vertices) {
		v.t = glm::normalize(v.t);
		v.b = glm::normalize(v.b);
	}

	return new Mesh(vertices, indices);
}

Mesh *loadMesh(const string &path) {
	fastObjMesh *obj = fast_obj_read(path.c_str());

	Mesh *mesh = loadMesh(obj, 0);

	fast_obj_destroy(obj);

	return mesh;
}
