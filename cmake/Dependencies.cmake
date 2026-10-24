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
 option(PENGUI_GPU_UPLOAD_FIX "Build pinned SDL with size-classed GPU vertex uploads" ON)
 option(PENGUI_VULKAN_DIAGNOSTICS "Instrument pinned SDL Vulkan backend for performance logs" OFF)
 if(NOT PENGUI_VULKAN_DIAGNOSTICS AND NOT PENGUI_GPU_UPLOAD_FIX)
  find_package(SDL3 3.4 CONFIG QUIET)
 endif()
 if(PENGUI_VULKAN_DIAGNOSTICS OR PENGUI_GPU_UPLOAD_FIX OR NOT SDL3_FOUND)
  if(NOT PENGUI_FETCH_SDL)
   message(FATAL_ERROR "Enable PENGUI_FETCH_SDL for the SDL upload fix/diagnostics, or disable both PENGUI_GPU_UPLOAD_FIX and PENGUI_VULKAN_DIAGNOSTICS to use installed SDL3 >= 3.4")
  endif()
  include(FetchContent)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  # XTEST is only needed for SDL's synthetic X11 input support. The apps do not
  # use it; disable it also in existing caches after the SDL 3.4 upgrade.
  set(SDL_X11_XTEST OFF CACHE BOOL "Unused synthetic X11 input support" FORCE)
  FetchContent_Declare(SDL3 URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz
   URL_HASH SHA256=9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3)
  if(PENGUI_VULKAN_DIAGNOSTICS OR PENGUI_GPU_UPLOAD_FIX)
   find_package(Python3 REQUIRED COMPONENTS Interpreter)
   FetchContent_GetProperties(SDL3)
   if(NOT sdl3_POPULATED)
    FetchContent_Populate(SDL3)
   endif()
   # Never modify an installed SDL or a user-provided source override.
   set(_pui_sdl_copy "${CMAKE_BINARY_DIR}/sdl-pengui")
   file(COPY "${sdl3_SOURCE_DIR}/" DESTINATION "${_pui_sdl_copy}")
   foreach(_pui_patch IN ITEMS fix_gpu_uploads instrument_vulkan)
    if(_pui_patch STREQUAL "fix_gpu_uploads")
     set(_pui_patch_source src/render/gpu/SDL_render_gpu.c)
     set(_pui_patch_enabled ${PENGUI_GPU_UPLOAD_FIX})
    else()
     set(_pui_patch_source src/gpu/vulkan/SDL_gpu_vulkan.c)
     set(_pui_patch_enabled ${PENGUI_VULKAN_DIAGNOSTICS})
    endif()
    # COPY can retain newer patched files: reset even when a patch is disabled.
    configure_file("${sdl3_SOURCE_DIR}/${_pui_patch_source}"
     "${_pui_sdl_copy}/${_pui_patch_source}" COPYONLY)
    if(_pui_patch_enabled)
     set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
      "${PROJECT_SOURCE_DIR}/tools/sdl/${_pui_patch}.py")
     execute_process(COMMAND "${Python3_EXECUTABLE}"
      "${PROJECT_SOURCE_DIR}/tools/sdl/${_pui_patch}.py" "${_pui_sdl_copy}"
      RESULT_VARIABLE _pui_patch_result)
     if(NOT _pui_patch_result EQUAL 0)
      message(FATAL_ERROR "SDL patch ${_pui_patch} failed: pinned source does not match")
     endif()
    endif()
   endforeach()
   add_subdirectory("${_pui_sdl_copy}" "${sdl3_BINARY_DIR}")
  else()
   FetchContent_MakeAvailable(SDL3)
  endif()
 endif()
endif()
