add_library(penguin_vector STATIC
 document/document.c
 document/boolean.c
 document/svg.c
 document/metadata.c
 document/history.c
 viewport/preview_view.c
 viewport/grid_view.c
 document/nanosvg.c)
target_include_directories(penguin_vector PUBLIC ${PROJECT_SOURCE_DIR}/src PRIVATE ${PROJECT_SOURCE_DIR}/third_party/cjson ${PROJECT_SOURCE_DIR}/third_party/nanosvg)
target_link_libraries(penguin_vector PUBLIC penguin)
if(NOT MSVC)
 set_source_files_properties(document/document.c document/boolean.c document/svg.c document/metadata.c document/history.c
 viewport/preview_view.c
 viewport/grid_view.c
  PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic")
endif()
if(PENGUIN_BUILD_EDITOR)
 target_sources(penguin_editor PRIVATE
  adapter.c
  editor.c
  panels/tree.c
  panels/panels.c
  tools/fields.c
  tools/actions.c
  tools/input.c
  viewport/render.c
  viewport/preview.c
  viewport/grid.c
  checkpoint.c
  demo.c)
endif()
