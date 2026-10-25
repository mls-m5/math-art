cmake_minimum_required(VERSION 3.23)
project(blumme LANGUAGES CXX)

# Noise and mesh math are expensive without optimization. Respect an explicit
# Debug choice, but do not silently produce unoptimized native builds by default.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

if(EMSCRIPTEN)
    if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/site)
    endif()
    set(BUILD_TESTING OFF CACHE BOOL "Build native geometry tests")
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        INTERFACE_COMPILE_OPTIONS "-sUSE_SDL=2"
        INTERFACE_LINK_OPTIONS "-sUSE_SDL=2")
    add_library(OpenGL::GL INTERFACE IMPORTED)
    add_compile_options(-fexceptions)
    add_link_options(-fexceptions -sALLOW_MEMORY_GROWTH=1
        -sGL_ENABLE_GET_PROC_ADDRESS=1 -sMIN_WEBGL_VERSION=1 -sMAX_WEBGL_VERSION=1
        -sENVIRONMENT=web)
else()
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)
endif()
include(CTest)
add_subdirectory(lib)

add_library(voronoi src/voronoi.cpp src/flower.cpp)
target_include_directories(voronoi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(voronoi PUBLIC cxx_std_17)
target_link_libraries(voronoi PUBLIC graphics_types)

include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
set(GLM_BUILD_TESTS OFF CACHE BOOL "Build GLM tests")
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "Use header-only GLM")
FetchContent_Declare(glm
    URL https://codeload.github.com/g-truc/glm/tar.gz/refs/tags/1.0.1
    URL_HASH SHA256=9f3174561fd26904b23f0db5e560971cbf9b3cbda0b280f04d5c379d03bf234c)
FetchContent_MakeAvailable(glm)
add_library(space_model src/space.cpp)
target_include_directories(space_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(space_model PUBLIC cxx_std_17)
target_link_libraries(space_model PUBLIC glm::glm graphics_types)

add_executable(blumme src/main.cpp)
# Embed editable GLSL files and reconfigure whenever they change.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    src/shaders/rays.vert src/shaders/rays.frag)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/rays.vert RAYS_VERTEX_SOURCE)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/rays.frag RAYS_FRAGMENT_SOURCE)
configure_file(src/shaders/rays_shaders.h.in generated/rays_shaders.h @ONLY)
add_executable(rays src/rays.cpp)
target_include_directories(rays PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
add_library(grid_geometry src/grid.cpp)
target_include_directories(grid_geometry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(grid_geometry PUBLIC cxx_std_17)
target_link_libraries(grid_geometry PUBLIC graphics_types)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    src/shaders/grid.vert src/shaders/grid.frag)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/grid.vert GRID_VERTEX_SOURCE)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/grid.frag GRID_FRAGMENT_SOURCE)
configure_file(src/shaders/grid_shaders.h.in generated/grid_shaders.h @ONLY)
add_executable(wonky_grid src/wonky_grid.cpp)
target_include_directories(wonky_grid PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(wonky_grid PRIVATE grid_geometry)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    src/shaders/space.vert src/shaders/space.frag)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/space.vert SPACE_VERTEX_SOURCE)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/space.frag SPACE_FRAGMENT_SOURCE)
configure_file(src/shaders/space_shaders.h.in generated/space_shaders.h @ONLY)
add_executable(space_travel src/space_travel.cpp)
target_include_directories(space_travel PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(space_travel PRIVATE space_model)

add_library(terrain_model src/terrain.cpp)
target_include_directories(terrain_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(terrain_model PUBLIC cxx_std_17)
target_link_libraries(terrain_model PUBLIC glm::glm graphics_types)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    src/shaders/terrain.vert src/shaders/terrain.frag)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/terrain.vert TERRAIN_VERTEX_SOURCE)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/terrain.frag TERRAIN_FRAGMENT_SOURCE)
configure_file(src/shaders/terrain_shaders.h.in generated/terrain_shaders.h @ONLY)
add_executable(isometric_world src/isometric_world.cpp)
target_include_directories(isometric_world PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_link_libraries(isometric_world PRIVATE terrain_model)

foreach(app IN ITEMS blumme rays wonky_grid space_travel isometric_world)
    # SDL2main supplies the platform entry point where SDL requires it.
    if(TARGET SDL2::SDL2main)
        target_link_libraries(${app} PRIVATE SDL2::SDL2main)
    endif()
    target_link_libraries(${app} PRIVATE graphics)
    if(EMSCRIPTEN)
        set_target_properties(${app} PROPERTIES SUFFIX ".html")
        target_link_options(${app} PRIVATE
            "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
        set_property(TARGET ${app} APPEND PROPERTY LINK_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html)
    endif()
endforeach()
target_link_libraries(blumme PRIVATE voronoi)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if(EMSCRIPTEN)
    # Relative links work at both / and /repository/ on GitHub Pages.
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        web/index.html web/gallery.css web/.nojekyll)
    file(COPY web/index.html web/gallery.css web/.nojekyll
        DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()
