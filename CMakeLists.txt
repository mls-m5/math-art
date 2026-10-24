
cmake_minimum_required(VERSION 3.23)
project(blumme)

enable_testing()
add_subdirectory(lib)

add_library(voronoi src/voronoi.cpp src/flower.cpp)
target_include_directories(voronoi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(voronoi PUBLIC cxx_std_17)

add_executable(
    blumme
    src/main.cpp
    )

target_compile_features(
    blumme
    PRIVATE
    cxx_std_17
    )

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

# SDL2main supplies the platform entry point where SDL requires it.
if(TARGET SDL2::SDL2main)
    target_link_libraries(blumme PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(blumme PRIVATE voronoi SDL2::SDL2 OpenGL::GL)

add_subdirectory(test)

