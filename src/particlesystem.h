#pragma once

#include "utils.h"
#include "assets.h"
#include "random.h"

template <typename T>
struct Prop {
	T minVal;
	T maxVal;

	Prop(const T &minVal, const T &maxVal): 
		minVal(minVal), 
		maxVal(maxVal) { }

	Prop(const Prop &p):
		Prop(p.minVal, p.maxVal) { }

	Prop(const T &val): 
		Prop(val, val) { }

	T val(Random &r) {
		return r.next(minVal, maxVal);
	}
};

template <typename T>
struct AnimProp {
	Prop<T> startVal;
	Prop<T> endVal;

	float interpExp;

	AnimProp(const Prop<T> &startVal, const Prop<T> &endVal, float exp = 1.f): 
		startVal(startVal),
		endVal(endVal),
		interpExp(exp) { }

	AnimProp(const AnimProp &p):
		AnimProp(p.startVal, p.endVal, p.interpExp) { }

	AnimProp(const Prop<T> &val, float exp = 1.f):
		AnimProp(val, val, exp) { }

	T val(Random &r, float k) {
		k = powf(k, interpExp);
		T start = startVal.val(r);
		T end = endVal.val(r);
		return (1.f - k) * start + k * end;
	}

};

struct Particle {
	vec3 pos;
	vec3 scale;
	vec3 color;
	float opacity;
};

struct ParticleSystem {
	Prop<float> life = Prop<float>(3.f);
	Prop<vec3> pos = Prop<vec3>(vec3(0), vec3(1));
	Prop<vec3> vel = Prop<vec3>(vec3(1));
	Prop<vec3> acc = Prop<vec3>(vec3(0));
	AnimProp<vec3> scale = AnimProp<vec3>(vec3(1.f));
	AnimProp<vec3> color = AnimProp<vec3>(vec3(1));
	AnimProp<float> opacity = AnimProp<float>(1.f, 0.f);
	Prop<float> fadeIn = Prop<float>(.1f);

	u32 count;

	Texture *texture;

	Particle simulate(Random &r, double time) {
		float cycle = (life.val(r));
		float t = (float)glm::mod(time, (double)cycle);
		float k = t / cycle;
		Particle p;
		vec3 p0 = pos.val(r);
		vec3 v0 = vel.val(r);
		vec3 a = acc.val(r);
		p.pos = p0 + t*(v0 + (.5f*t)*a);
		p.scale = scale.val(r, k);
		p.color = color.val(r, k);
		p.opacity = opacity.val(r, k) * glm::min(t/fadeIn.val(r), 1.0f);;


		return p;
	}

	void draw(const Particle &p) {

	}

	void draw(double time) {
		Random r(23);

		for (size_t i = 0; i < count; i++) {
			Particle p = simulate(r, time);
			draw(p);
		}
	}

};