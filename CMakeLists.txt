cmake_minimum_required(VERSION 3.16)
project(gidemo)

set(CMAKE_CXX_STANDARD 17)
set(TARGET_NAME gidemo)
file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp" "src/*.h")
add_executable(gidemo ${PROJECT_SOURCES})

if(MSVC)
  target_compile_options(gidemo PRIVATE /W4 /WX)
else()
  target_compile_options(gidemo PRIVATE -Wall -Wextra -pedantic )
endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/glfw)

add_library(glad libs/glad/src/glad.c)

include_directories(libs/glad/include libs/glm libs/stb libs/fast_obj)

target_link_libraries(gidemo glfw glad)
