
#include <stdio.h>

#include "utils.h"

bool readFile(const std::string &path, std::string &s) {
	std::FILE* f;
	f = fopen(path.c_str(), "rb");
	if (!f) return false;

	fseek(f, 0, SEEK_END);
	s.resize(ftell(f));
	rewind(f);
	fread(&s[0], 1, s.size(), f);
	if (ferror(f)) return false;
	fclose(f);

	return true;
}

#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

u32 MurmurHash2 ( const void * key, int len, u32 seed ) {
	const u32 m = 0x5bd1e995;
	const i32 r = 24;
	u32 h = seed ^ len;
	const u8 *data = (const u8 *)key;

	while(len >= 4) {
		u32 k = *(u32 *)data;
		k *= m; 
		k ^= k >> r; 
		k *= m; 
		h *= m; 
		h ^= k;
		data += 4;
		len -= 4;
	}
	
	switch(len) {
		case 3: h ^= data[2] << 16;
		case 2: h ^= data[1] << 8;
		case 1: h ^= data[0]; 
		h *= m;
	};

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
} 
#pragma GCC diagnostic pop

u32 hash(const void *buffer, int size) {
	return MurmurHash2(buffer, size, 0xa5a8ae1e);
}
