cmake_minimum_required(VERSION 3.16)
set(MSVC_INCREMENTAL_DEFAULT ON)
project(gidemo)

# set(CMAKE_CXX_STANDARD 17)
set(TARGET_NAME gidemo)
file(GLOB_RECURSE PROJECT_SOURCES "src/*.cpp" "src/*.h")
add_executable(gidemo ${PROJECT_SOURCES})


if(MSVC)
  target_compile_options(gidemo PRIVATE /W4)
else()
  target_compile_options(gidemo PRIVATE -Wall -Wextra -pedantic )
endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/glfw EXCLUDE_FROM_ALL)

#set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
#set(BUILD_CLSOCKET OFF CACHE BOOL "" FORCE)
#set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
#set(BUILD_ENET OFF CACHE BOOL "" FORCE)
#set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
#set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
#set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
#set(USE_GLUT OFF CACHE BOOL "" FORCE)
#set(USE_GRAPHICAL_BENCHMARK OFF CACHE BOOL "" FORCE)
#add_subdirectory(libs/bullet3 EXCLUDE_FROM_ALL)

add_library(glad libs/glad/src/glad.c)

include_directories(libs/glad/include libs/glm libs/stb libs/fast_obj)

# target_link_libraries(gidemo glfw glad Bullet3Common BulletCollision BulletDynamics LinearMath)
target_link_libraries(gidemo glfw glad)
