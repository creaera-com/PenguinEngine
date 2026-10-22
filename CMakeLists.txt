cmake_minimum_required(VERSION 3.20)
if(POLICY CMP0135)
 cmake_policy(SET CMP0135 NEW)
endif()
project(pengUI VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
option(PENGUI_BUILD_APP "Build the playground and editor-shell example applications" ON)
option(PENGUI_BUILD_SDL "Build the SDL3 renderer" ON)
option(PENGUI_FETCH_SDL "Fetch SDL3 when not installed" ON)
option(PENGUI_SANITIZERS "Enable ASan and UBSan" OFF)
if(MSVC)
 # Geometry operations use bounded shape snapshots. Match the usual 8 MiB
 # desktop stack reserve; MSVC otherwise gives executables only 1 MiB.
 # This reserves address space, without committing the full amount of RAM.
 add_link_options(/STACK:8388608)
endif()
if(PENGUI_BUILD_APP AND NOT PENGUI_BUILD_SDL)
 message(FATAL_ERROR "The playground requires PENGUI_BUILD_SDL=ON")
endif()
if(PENGUI_SANITIZERS AND NOT MSVC)
 add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
 add_link_options(-fsanitize=address,undefined)
endif()
include(CTest)
include(cmake/Dependencies.cmake)
add_subdirectory(lib/pengdraw)
add_subdirectory(lib/pengui)
if(PENGUI_BUILD_APP)
 add_subdirectory(apps)
endif()
if(BUILD_TESTING)
 add_subdirectory(tests)
endif()

# Documentation is optional and never required by normal application builds.
find_package(Doxygen 1.9.7 QUIET)
if(DOXYGEN_FOUND)
 configure_file(docs/doxygen/Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile @ONLY)
 add_custom_target(api_docs
  COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  COMMENT "Generating PenguinEngine, pengUI and pengDraw documentation"
  VERBATIM)
endif()
