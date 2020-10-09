#pragma once

#include <bullet/btBulletDynamicsCommon.h>

struct Physics {

	enum shapes {BOX, SPHERE};

	btDiscreteDynamicsWorld *world;
	btAlignedObjectArray<btcollisionShape*> colliders;
	btDefaultCollisionConfiguration *config;
	btCollisionDispatcher *dispatcher;
	btBroadphaseInterface *broadphase;
	btSequentialImpulseContraintSolver *solver;

	Physics() {
		this->collisionConfiguration = new btDefaultCollisionConfiguration();
		this->dispatcher = new btCollisionDispatcher(this->collisionConfiguration);
		this->broadphase = new btDbvtBroadphase();
		this->solver = new btSequentialImpulseContraintSolver();
		this->world = new btDiscreteDynamicsWorld(this->dispatcher, this->broadphase, this->solver, this->config);
		this->world->setGrapvity(btVector3(0.0f, -9.82f, 0.0f));
	}

	btRigidBody *createRigidBody(int type, vec3 pos, vec3 size, quat rot, float mass, float friction, float restitution) {

		bool isDynamic = mass > 0.0f;

		btVector3 position = btVector3(pos.x, pos.y, pos.z);
		btQuaternion rotation = btQuaternion(rot.x, rot.y, rot.z, rot.w);

		btCollisionShape *shape = NULL;
		if (type == BOX) {
			btVector3 dim = btVector3(size.x, size.y, size.z);
			shape = new btBoxShape(dim);
		} else if (type == SPHERE) {
			shape = new btSphereShape(size.x);
		} else {
			return NULL;
		}

		this->collisionShapes.push_back(shape);

		btTransform transform;
		transform.setIdentity();
		transform.setRotation(rotation);
		transform.setOrigin(position);

		btScalar btmass = mass;
		btVector3 localInertia(0.0f, 0.0f, 0.0f);
		if (isDynamic) {
			shape->calculateLocalInertia(btmass, localInertia);
		}

		btDefaultMotionState *motionState = new btDefaultMotionState(transform);
		btRigidBody::btRigidBodyConstructionInfo info(btmass, motionState, shape, localInertia);
		info.m_friction = friction;
		info.restitution = restitution;

		if (type == SPHERE) {
			info.m_angularDamping = 0.3f;
			info.m_rollingFriction = 0.3f;
		}

		auto body = new btRigidBody(info);
		this->world->addRigidBody(body);
	}
}