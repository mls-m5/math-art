cmake_minimum_required(VERSION 3.23)
project(blumme LANGUAGES CXX)

enable_testing()
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
add_subdirectory(lib)

add_library(voronoi src/voronoi.cpp src/flower.cpp)
target_include_directories(voronoi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(voronoi PUBLIC cxx_std_17)
target_link_libraries(voronoi PUBLIC graphics_types)

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

foreach(app IN ITEMS blumme rays wonky_grid)
    # SDL2main supplies the platform entry point where SDL requires it.
    if(TARGET SDL2::SDL2main)
        target_link_libraries(${app} PRIVATE SDL2::SDL2main)
    endif()
    target_link_libraries(${app} PRIVATE graphics)
endforeach()
target_link_libraries(blumme PRIVATE voronoi)

add_subdirectory(test)
