# Vendored QuickJS-ng: parser only, no CLI or OS modules.
find_package(Threads REQUIRED)
add_library(penguin_js_parser STATIC
 ${PROJECT_SOURCE_DIR}/third_party/quickjs-ng/quickjs.c ${PROJECT_SOURCE_DIR}/third_party/quickjs-ng/cutils.c
 ${PROJECT_SOURCE_DIR}/third_party/quickjs-ng/libregexp.c ${PROJECT_SOURCE_DIR}/third_party/quickjs-ng/libunicode.c
 ${PROJECT_SOURCE_DIR}/third_party/quickjs-ng/xsum.c)
set_target_properties(penguin_js_parser PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
target_include_directories(penguin_js_parser PUBLIC ${PROJECT_SOURCE_DIR}/third_party/quickjs-ng)
target_compile_definitions(penguin_js_parser PRIVATE _GNU_SOURCE)
target_link_libraries(penguin_js_parser PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
if(MSVC)
 target_compile_options(penguin_js_parser PRIVATE /experimental:c11atomics /J)
 target_compile_definitions(penguin_js_parser PRIVATE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)
else()
 target_link_libraries(penguin_js_parser PUBLIC m)
 target_compile_options(penguin_js_parser PRIVATE -funsigned-char)
endif()
add_library(penguin_yaml STATIC ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/api.c ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/reader.c
 ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/scanner.c ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/parser.c ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/loader.c
 ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/writer.c ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/emitter.c ${PROJECT_SOURCE_DIR}/third_party/libyaml/src/dumper.c)
target_include_directories(penguin_yaml PUBLIC ${PROJECT_SOURCE_DIR}/third_party/libyaml/include)
target_compile_definitions(penguin_yaml PUBLIC YAML_DECLARE_STATIC PRIVATE
 YAML_VERSION_MAJOR=0 YAML_VERSION_MINOR=2 YAML_VERSION_PATCH=5 YAML_VERSION_STRING="0.2.5")
if(MSVC)
 target_compile_definitions(penguin_yaml PRIVATE strdup=_strdup)
else()
 target_compile_definitions(penguin_yaml PRIVATE _POSIX_C_SOURCE=200809L)
endif()
add_library(penguin_vterm STATIC
 ${PROJECT_SOURCE_DIR}/third_party/libvterm/src/encoding.c ${PROJECT_SOURCE_DIR}/third_party/libvterm/src/keyboard.c
 ${PROJECT_SOURCE_DIR}/third_party/libvterm/src/mouse.c ${PROJECT_SOURCE_DIR}/third_party/libvterm/src/parser.c
 ${PROJECT_SOURCE_DIR}/third_party/libvterm/src/pen.c ${PROJECT_SOURCE_DIR}/third_party/libvterm/src/screen.c
 ${PROJECT_SOURCE_DIR}/third_party/libvterm/src/state.c ${PROJECT_SOURCE_DIR}/third_party/libvterm/src/unicode.c ${PROJECT_SOURCE_DIR}/third_party/libvterm/src/vterm.c)
target_include_directories(penguin_vterm PUBLIC ${PROJECT_SOURCE_DIR}/third_party/libvterm/include)

if(PENGUIN_BUILD_EDITOR)
 find_package(SDL3 3.2 CONFIG QUIET)
 if(NOT SDL3_FOUND)
  if(NOT PENGUIN_FETCH_SDL)
   message(FATAL_ERROR "SDL3 >= 3.2 required. Install SDL3 or enable PENGUIN_FETCH_SDL.")
  endif()
  include(FetchContent)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL3 URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.10/SDL3-3.2.10.tar.gz URL_HASH SHA256=f87be7b4dec66db4098e9c167b2aa34e2ca10aeb5443bdde95ae03185ed513e0)
  FetchContent_MakeAvailable(SDL3)
 endif()
endif()
