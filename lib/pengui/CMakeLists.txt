# Core widget behavior and YAML stay independent of SDL.
add_library(pengUI STATIC
 src/core/context.c
 src/widgets/button.c src/widgets/basic.c src/widgets/text_input.c
 src/widgets/tabs.c src/widgets/tree.c src/widgets/scrollbar.c src/widgets/splitter.c
 src/layout/layout.c src/layout/yaml.c)
target_include_directories(pengUI PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_library(pengUI::core ALIAS pengUI)
target_compile_features(pengUI PUBLIC c_std_17)
target_include_directories(pengUI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(pengUI PRIVATE pengui_yaml_parser)
if(NOT MSVC)
 target_compile_options(pengUI PRIVATE -Wall -Wextra -Wpedantic -Werror)
 target_link_libraries(pengUI PUBLIC m)
endif()
if(PENGUI_BUILD_SDL)
 # SDL counterparts contain each widget's drawing and native input handling.
 add_library(pengUI_sdl STATIC
  src/rendering/text.c src/rendering/icons.c src/rendering/theme.c
  src/rendering/image.c src/rendering/primitives.c
  src/widgets/button_sdl.c src/widgets/text_input_sdl.c src/widgets/basic_sdl.c
  src/widgets/tabs_sdl.c src/widgets/tree_sdl.c src/widgets/scrollbar_sdl.c src/widgets/splitter_sdl.c
  src/layout/layout_sdl.c src/view/view.c src/view/events.c src/view/draw.c src/window.c)
 target_include_directories(pengUI_sdl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
 add_library(pengUI::sdl ALIAS pengUI_sdl)
 target_compile_definitions(pengUI_sdl PUBLIC PENGUI_WITH_SDL=1)
 target_include_directories(pengUI_sdl SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/stb)
 target_link_libraries(pengUI_sdl PUBLIC pengUI SDL3::SDL3)
 if(NOT MSVC)
  target_compile_options(pengUI_sdl PRIVATE -Wall -Wextra -Wpedantic -Werror)
 endif()
endif()
