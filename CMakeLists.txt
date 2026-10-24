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
add_executable(rays src/rays.cpp)
foreach(app IN ITEMS blumme rays)
    # SDL2main supplies the platform entry point where SDL requires it.
    if(TARGET SDL2::SDL2main)
        target_link_libraries(${app} PRIVATE SDL2::SDL2main)
    endif()
    target_link_libraries(${app} PRIVATE graphics)
endforeach()
target_link_libraries(blumme PRIVATE voronoi)

add_subdirectory(test)
