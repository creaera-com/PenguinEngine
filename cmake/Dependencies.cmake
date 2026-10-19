add_library(pengui_yaml_parser STATIC
 ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/api.c
 ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/reader.c
 ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/scanner.c
 ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/parser.c
 ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/loader.c
 ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/writer.c
 ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/emitter.c
 ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/dumper.c)
target_include_directories(pengui_yaml_parser PUBLIC ${PROJECT_SOURCE_DIR}/third_party/libyaml/include)
target_compile_definitions(pengui_yaml_parser PUBLIC YAML_DECLARE_STATIC PRIVATE
 YAML_VERSION_MAJOR=0 YAML_VERSION_MINOR=2 YAML_VERSION_PATCH=5 YAML_VERSION_STRING="0.2.5")
if(MSVC)
 target_compile_definitions(pengui_yaml_parser PRIVATE strdup=_strdup)
else()
 target_compile_definitions(pengui_yaml_parser PRIVATE _POSIX_C_SOURCE=200809L)
endif()
if(PENGUI_BUILD_SDL)
 find_package(SDL3 3.2 CONFIG QUIET)
 if(NOT SDL3_FOUND)
  if(NOT PENGUI_FETCH_SDL)
   message(FATAL_ERROR "SDL3 >= 3.2 required; install SDL3 or enable PENGUI_FETCH_SDL")
  endif()
  include(FetchContent)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL3 URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.10/SDL3-3.2.10.tar.gz
   URL_HASH SHA256=f87be7b4dec66db4098e9c167b2aa34e2ca10aeb5443bdde95ae03185ed513e0)
  FetchContent_MakeAvailable(SDL3)
 endif()
endif()

if(PENGUI_BUILD_APP)
# SGI Free Software License B; pinned vendored C source, no network needed when building.
add_library(pengui_tess STATIC
 ${PROJECT_SOURCE_DIR}/third_party/libtess2/Source/bucketalloc.c
 ${PROJECT_SOURCE_DIR}/third_party/libtess2/Source/dict.c
 ${PROJECT_SOURCE_DIR}/third_party/libtess2/Source/geom.c
 ${PROJECT_SOURCE_DIR}/third_party/libtess2/Source/mesh.c
 ${PROJECT_SOURCE_DIR}/third_party/libtess2/Source/priorityq.c
 ${PROJECT_SOURCE_DIR}/third_party/libtess2/Source/sweep.c
 ${PROJECT_SOURCE_DIR}/third_party/libtess2/Source/tess.c)
target_include_directories(pengui_tess PUBLIC ${PROJECT_SOURCE_DIR}/third_party/libtess2/Include)
if(NOT MSVC)
 target_link_libraries(pengui_tess PUBLIC m)
endif()
endif()
