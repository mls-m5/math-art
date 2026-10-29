add_executable(voronoi_test voronoi_test.cpp)
target_link_libraries(voronoi_test PRIVATE voronoi)
add_test(NAME voronoi_geometry COMMAND voronoi_test)

add_executable(rays_test rays_test.cpp)
target_link_libraries(rays_test PRIVATE graphics_types)
target_include_directories(rays_test PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_features(rays_test PRIVATE cxx_std_17)
add_test(NAME rays_geometry COMMAND rays_test)

add_executable(grid_test grid_test.cpp)
target_link_libraries(grid_test PRIVATE grid_geometry)
add_test(NAME grid_geometry COMMAND grid_test)

add_executable(space_test space_test.cpp)
target_link_libraries(space_test PRIVATE space_model)
add_test(NAME space_geometry COMMAND space_test)

add_executable(terrain_test terrain_test.cpp)
target_link_libraries(terrain_test PRIVATE terrain_model)
add_test(NAME terrain_geometry COMMAND terrain_test)

add_executable(simplex_texture_test simplex_texture_test.cpp)
target_link_libraries(simplex_texture_test PRIVATE simplex_textures)
add_test(NAME simplex_textures COMMAND simplex_texture_test)
