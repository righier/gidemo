#pragma once

#include <cinttypes>
#include <string>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)
#pragma warning(disable: 4127)
#endif
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/type_ptr.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat2;
using glm::mat3;
using glm::mat4;
using glm::quat;

const float pi = 3.14159265358979323846f;

#include "log.h"

typedef std::int8_t i8;
typedef std::uint8_t u8;
typedef std::int16_t i16;
typedef std::uint16_t u16;
typedef std::int32_t i32;
typedef std::uint32_t u32;
typedef std::int64_t i64;
typedef std::uint64_t u64;

using std::vector;
using std::string;

#define UNUSED(x) (void)(x)

/* read an entire file into a string */
bool readFile(const string &path, string &out);

/* generate a 32 bit hash from a variable length buffer */
u32 hash(const void *buffer, int size);

/* string wstring conversion */
std::wstring s2ws(const std::string &str);
std::string ws2s(const std::wstring& wstr);

/* random float between 0 and 1 */
float randomf();
