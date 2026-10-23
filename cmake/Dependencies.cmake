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
 find_package(SDL3 3.4 CONFIG QUIET)
 if(NOT SDL3_FOUND)
  if(NOT PENGUI_FETCH_SDL)
   message(FATAL_ERROR "SDL3 >= 3.4 required; install SDL3 or enable PENGUI_FETCH_SDL")
  endif()
  include(FetchContent)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL3 URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz
   URL_HASH SHA256=9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3)
  FetchContent_MakeAvailable(SDL3)
 endif()
endif()
