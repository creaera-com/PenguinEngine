# Independent geometry and SDL backend: no pengUI, app, or editor dependency.
cmake_minimum_required(VERSION 3.20)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
 project(pengDraw VERSION 0.1.0 LANGUAGES C)
endif()
set(_pdraw_sdl_default ON)
if(DEFINED PENGUI_BUILD_SDL)
 set(_pdraw_sdl_default ${PENGUI_BUILD_SDL})
endif()
option(PENGDRAW_BUILD_SDL "Build the independent pengDraw SDL3 backend" ${_pdraw_sdl_default})
if(PENGDRAW_BUILD_SDL AND NOT TARGET SDL3::SDL3)
 find_package(SDL3 3.4 CONFIG REQUIRED)
endif()
get_filename_component(PENGDRAW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
add_library(pengdraw_tess STATIC
 "${PENGDRAW_ROOT}/third_party/libtess2/Source/bucketalloc.c"
 "${PENGDRAW_ROOT}/third_party/libtess2/Source/dict.c"
 "${PENGDRAW_ROOT}/third_party/libtess2/Source/geom.c"
 "${PENGDRAW_ROOT}/third_party/libtess2/Source/mesh.c"
 "${PENGDRAW_ROOT}/third_party/libtess2/Source/priorityq.c"
 "${PENGDRAW_ROOT}/third_party/libtess2/Source/sweep.c"
 "${PENGDRAW_ROOT}/third_party/libtess2/Source/tess.c")
target_include_directories(pengdraw_tess PUBLIC "${PENGDRAW_ROOT}/third_party/libtess2/Include")
add_library(pengDraw STATIC src/geometry/path.c src/geometry/tessellation.c src/geometry/bezier.c
 src/geometry/rotation.c src/geometry/shape.c src/paint/gradient.c src/paint/shadow.c src/paint/text.c src/paint/text_layout.c)
add_library(pengDraw::core ALIAS pengDraw)
target_include_directories(pengDraw PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(pengDraw PUBLIC c_std_17)
target_link_libraries(pengDraw PRIVATE pengdraw_tess)
if(NOT MSVC)
 target_compile_options(pengDraw PRIVATE -Wall -Wextra -Wpedantic -Werror)
 target_link_libraries(pengDraw PUBLIC m)
endif()
if(PENGDRAW_BUILD_SDL)
 add_library(pengDraw_sdl STATIC src/sdl/path.c src/sdl/ellipse.c src/sdl/curve.c
  src/sdl/stroke.c src/sdl/gradient.c src/sdl/shape.c src/sdl/texture.c src/sdl/renderer.c src/sdl/mask.c src/sdl/shadow.c src/gpu/renderer.c src/gpu/shadow.c src/gpu/gradient.c)
 add_library(pengDraw::sdl ALIAS pengDraw_sdl)
 target_link_libraries(pengDraw_sdl PUBLIC pengDraw SDL3::SDL3)
 target_compile_definitions(pengDraw_sdl PUBLIC PDRAW_WITH_SDL=1)
 if(NOT MSVC)
  target_compile_options(pengDraw_sdl PRIVATE -Wall -Wextra -Wpedantic -Werror)
 endif()
endif()
