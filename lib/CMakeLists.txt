# Point is shared with CPU-only geometry without pulling in SDL/OpenGL.
add_library(graphics_types INTERFACE)
target_include_directories(graphics_types INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(graphics graphics/graphics.cpp graphics/shader.cpp)
target_compile_features(graphics PUBLIC cxx_std_17)
target_link_libraries(graphics PUBLIC graphics_types SDL2::SDL2 PRIVATE OpenGL::GL)
