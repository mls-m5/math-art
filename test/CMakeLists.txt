add_executable(voronoi_test voronoi_test.cpp)
target_link_libraries(voronoi_test PRIVATE voronoi)
add_test(NAME voronoi_geometry COMMAND voronoi_test)
