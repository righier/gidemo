#pragma once

#include <iostream>
#include "math.h"
#include "utils.h"

inline void LOGA(const vec2& v) {
	std::cout << "(" << v.x << "," << v.y << ")";
}

inline void LOGA(const vec3& v) {
	std::cout << "(" << v.x << "," << v.y << "," << v.z << ")";
}

inline void LOGA(const vec4& v) {
	std::cout << "(" << v.x << "," << v.y << "," << v.z << "," << v.w << ")";
}

inline void LOGA(const mat4& m) {
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			std::cout << m[y][x] << " ";
		}
		std::cout << "\n";
	}
}

template<typename First>
inline void LOGA(First arg) {
	std::cout << arg;
}

inline void LOG() {
	std::cout << std::endl;
}

template<typename First, typename ... Rest>
inline void LOG(First arg, const Rest&... rest) {
	LOGA(arg);
	std::cout << " ";
	LOG(rest...);
}


