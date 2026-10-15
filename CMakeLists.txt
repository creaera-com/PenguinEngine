cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0135)
 cmake_policy(SET CMP0135 NEW)
endif()
project(PenguinEngine VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
option(PENGUIN_BUILD_EDITOR "Build SDL3 editor and game" ON)
option(PENGUIN_FETCH_SDL "Download pinned SDL3 if not installed" ON)
option(PENGUIN_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
add_library(penguin STATIC
 src/engine/scene/scene.c
 src/engine/ecs/entities.c
 src/engine/tilemap/tilemap.c
 src/engine/history/history.c
 src/engine/camera/camera.c
 src/engine/collision/collision.c
 src/engine/runtime/update.c
 src/engine/scene/serialization.c
 src/engine/project/project.c
 src/engine/project/manifest.c
 src/shared/json/json_io.c
 third_party/cjson/cJSON.c)
target_include_directories(penguin PUBLIC src/engine/include PRIVATE src third_party/cjson)
if(NOT MSVC)
 target_compile_options(penguin PRIVATE -Wall -Wextra -Wpedantic)
 target_link_libraries(penguin PUBLIC m)
endif()
if(PENGUIN_SANITIZERS AND NOT MSVC)
 add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
 add_link_options(-fsanitize=address,undefined)
 target_compile_options(penguin PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()
# Vendored QuickJS-ng: parser only, no CLI or OS modules.
find_package(Threads REQUIRED)
add_library(penguin_js_parser STATIC
 third_party/quickjs-ng/quickjs.c third_party/quickjs-ng/cutils.c
 third_party/quickjs-ng/libregexp.c third_party/quickjs-ng/libunicode.c
 third_party/quickjs-ng/xsum.c)
set_target_properties(penguin_js_parser PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
target_include_directories(penguin_js_parser PUBLIC third_party/quickjs-ng)
target_compile_definitions(penguin_js_parser PRIVATE _GNU_SOURCE)
target_link_libraries(penguin_js_parser PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
if(MSVC)
 target_compile_options(penguin_js_parser PRIVATE /experimental:c11atomics /J)
 target_compile_definitions(penguin_js_parser PRIVATE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)
else()
 target_link_libraries(penguin_js_parser PUBLIC m)
 target_compile_options(penguin_js_parser PRIVATE -funsigned-char)
endif()
add_library(penguin_script STATIC src/engine/scripting/text.c src/engine/scripting/javascript.c)
target_include_directories(penguin_script PUBLIC src/engine/include)
target_link_libraries(penguin_script PRIVATE penguin_js_parser)
if(NOT MSVC)
 target_compile_options(penguin_script PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_library(penguin_yaml STATIC third_party/libyaml/src/api.c third_party/libyaml/src/reader.c
 third_party/libyaml/src/scanner.c third_party/libyaml/src/parser.c third_party/libyaml/src/loader.c
 third_party/libyaml/src/writer.c third_party/libyaml/src/emitter.c third_party/libyaml/src/dumper.c)
target_include_directories(penguin_yaml PUBLIC third_party/libyaml/include)
target_compile_definitions(penguin_yaml PUBLIC YAML_DECLARE_STATIC PRIVATE
 YAML_VERSION_MAJOR=0 YAML_VERSION_MINOR=2 YAML_VERSION_PATCH=5 YAML_VERSION_STRING="0.2.5")
if(MSVC)
 target_compile_definitions(penguin_yaml PRIVATE strdup=_strdup)
else()
 target_compile_definitions(penguin_yaml PRIVATE _POSIX_C_SOURCE=200809L)
endif()
add_library(penguin_ui_layout STATIC src/editor/layout/ui_layout.c)
target_include_directories(penguin_ui_layout PUBLIC src/editor third_party/cjson)
target_link_libraries(penguin_ui_layout PUBLIC penguin PRIVATE penguin_yaml)
if(NOT MSVC)
 target_compile_options(penguin_ui_layout PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_library(penguin_editor_session STATIC src/editor/workspace/session.c)
target_include_directories(penguin_editor_session PUBLIC src/editor)
if(NOT MSVC)
 target_compile_options(penguin_editor_session PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_subdirectory(src/ui)
include(CTest)
if(BUILD_TESTING)
 add_library(unity STATIC third_party/unity/unity.c)
 target_include_directories(unity PUBLIC third_party/unity)
 add_executable(penguin_tests tests/test_engine.c)
 target_link_libraries(penguin_tests PRIVATE penguin unity)
 target_include_directories(penguin_tests PRIVATE third_party/cjson)
 add_test(NAME engine_unit_tests COMMAND penguin_tests)
 add_executable(penguin_script_tests tests/test_script.c)
 target_link_libraries(penguin_script_tests PRIVATE penguin_script unity)
 add_test(NAME script_unit_tests COMMAND penguin_script_tests)
 add_executable(penguin_ui_tests tests/test_ui.c)
 target_link_libraries(penguin_ui_tests PRIVATE penguin_ui unity)
 add_test(NAME ui_unit_tests COMMAND penguin_ui_tests)
 add_executable(penguin_editor_state_tests tests/test_editor_state.c)
 target_link_libraries(penguin_editor_state_tests PRIVATE penguin_editor_session penguin_ui_layout unity)
 target_compile_definitions(penguin_editor_state_tests PRIVATE PE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
 add_test(NAME editor_state_unit_tests COMMAND penguin_editor_state_tests)
endif()
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
 penguin_ui_add_sdl()
 add_library(penguin_sdl STATIC src/platform/sdl/scene_renderer.c)
 target_include_directories(penguin_sdl PUBLIC src/platform/sdl/include PRIVATE third_party/stb)
 target_link_libraries(penguin_sdl PUBLIC penguin SDL3::SDL3)
 add_library(penguin_editor_documents STATIC
  src/editor/editors/map/checkpoint.c
  src/editor/editors/scripts/checkpoint.c)
 target_include_directories(penguin_editor_documents PUBLIC src/editor)
 target_link_libraries(penguin_editor_documents PUBLIC penguin_editor_session penguin_sdl penguin_script penguin_ui)
 add_executable(penguin_editor
  src/editor/app/main.c
  src/editor/app/state.c
  src/editor/app/input.c
  src/editor/app/shell.c
  src/editor/app/ui_helpers.c
  src/editor/browser/file_tree.c
  src/editor/dialogs/fields.c
  src/editor/layout/bindings.c
  src/editor/layout/renderer.c
  src/editor/workspace/documents.c
  src/editor/workspace/lifecycle.c
  src/editor/editors/map/actions.c
  src/editor/editors/map/fields.c
  src/editor/editors/map/runtime.c
  src/editor/editors/map/input.c
  src/editor/editors/map/document.c
  src/editor/editors/map/tools.c
  src/editor/editors/map/panels.c
  src/editor/editors/map/canvas.c
  src/editor/editors/scripts/script_editor.c
  src/editor/editors/scripts/actions.c
  src/editor/editors/scripts/panels.c)
 if(BUILD_TESTING)
  target_sources(penguin_editor PRIVATE tests/editor_smoke.c)
  target_include_directories(penguin_editor PRIVATE tests)
  target_compile_definitions(penguin_editor PRIVATE PE_EDITOR_SMOKE_TESTS=1)
 endif()
 target_include_directories(penguin_editor PRIVATE src/editor)
 target_link_libraries(penguin_editor PRIVATE penguin_editor_documents penguin_sdl penguin_script penguin_ui_sdl penguin_editor_session penguin_ui_layout)
 add_executable(penguin_ui_demo examples/ui/main.c)
 target_link_libraries(penguin_ui_demo PRIVATE penguin_ui_sdl)
 add_executable(penguin_game examples/topdown/main.c)
 target_link_libraries(penguin_game PRIVATE penguin_sdl)
 foreach(target penguin_sdl penguin_editor_documents penguin_editor penguin_game)
  if(NOT MSVC)
   target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
 endforeach()
 add_custom_target(copy_example ALL
  COMMAND ${CMAKE_COMMAND} -DSOURCE=${CMAKE_SOURCE_DIR}/examples/topdown
   -DDEST=${CMAKE_BINARY_DIR}/example -P ${CMAKE_SOURCE_DIR}/cmake/CopyExample.cmake
  VERBATIM
 )
 add_custom_target(copy_editor_ui ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources/editor/ui ${CMAKE_BINARY_DIR}/ui-layouts
  VERBATIM)
 add_dependencies(penguin_editor copy_editor_ui)
 add_dependencies(penguin_editor copy_example)
 add_dependencies(penguin_game copy_example)
 if(BUILD_TESTING)
  add_executable(penguin_editor_document_tests tests/test_editor_documents.c)
  target_link_libraries(penguin_editor_document_tests PRIVATE penguin_editor_documents unity)
  add_test(NAME editor_document_unit_tests COMMAND penguin_editor_document_tests)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/unrelated-working-directory)
  add_test(NAME editor_default_project COMMAND penguin_editor --smoke)
  add_test(NAME game_default_project COMMAND penguin_game --smoke)
  set_tests_properties(editor_default_project game_default_project PROPERTIES
   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/unrelated-working-directory
   ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 20)
  add_test(NAME example_copy_preserves_edits COMMAND ${CMAKE_COMMAND}
   -DSOURCE=${CMAKE_SOURCE_DIR} -DBINARY=${CMAKE_BINARY_DIR}
   -P ${CMAKE_SOURCE_DIR}/tests/test_copy_example.cmake)
  add_test(NAME ui_demo_smoke COMMAND penguin_ui_demo --smoke)
  set_tests_properties(ui_demo_smoke PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 20)
  add_test(NAME editor_script_smoke COMMAND penguin_editor --smoke --scripts)
  set_tests_properties(editor_script_smoke PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 20)
  add_test(NAME editor_smoke COMMAND penguin_editor --project ${CMAKE_BINARY_DIR}/example --smoke)
  add_test(NAME game_smoke COMMAND penguin_game --project ${CMAKE_BINARY_DIR}/example --smoke)
  set_tests_properties(editor_smoke game_smoke PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 20)
 endif()
endif()
