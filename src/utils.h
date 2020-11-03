#pragma once

#include <inttypes.h>
#include <string>
#include <vector>

#pragma warning(push)
#pragma warning(disable: 4201)
#pragma warning(disable: 4127)
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/type_ptr.hpp>
#pragma warning(pop)

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat2;
using glm::mat3;
using glm::mat4;
using glm::quat;

const float pi = 3.14159265358979323846f;

#include "log.h"

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

using std::vector;
using std::string;

#define UNUSED(x) (void)(x)

bool readFile(const string &path, string &out);

u32 hash(const void *buffer, int size);

std::wstring s2ws(const std::string &str);
std::string ws2s(const std::wstring& wstr);


float randomf();
