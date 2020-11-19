#pragma once

#include "utils.h"

struct Random {
	u32 state[2];

	static u32 murmur3(u32 h) {
		h ^= h >> 16;
		h *= 0x85ebca6b;
		h ^= h >> 13;
		h *= 0xc2b2ae35;
		h ^= h >> 16;
		return h;
	}

	Random(u32 seed) {
		u32 v = murmur3((seed << 1U) | 1U);
		state[0] = v;
		state[1] = v ^ 0x49616e42U;
	}

	u32 nextInt() {
	    state[0] = (state[0] << 16) + (state[0] >> 16);
	    state[0] += state[1];
	    state[1] += state[0];
	    return state[0];	
	}

	template <typename T>
	T next();

	template <typename T>
	T next(const T &max) {
		return next<T>() * max;
	}

	template <typename T>
	T next(const T &min, const T &max) {
		return next(max - min) + min;
	}

};

template<>
inline float Random::next() {
	auto x = nextInt();
	return (float)x / (float)(1ULL<<32ULL);
}

template <>
inline vec3 Random::next() {
	return vec3(next<float>(), next<float>(), next<float>());
}
