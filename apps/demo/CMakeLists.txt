add_executable(pengui_playground main.c application.c)
target_link_libraries(pengui_playground PRIVATE pengUI::sdl)
set_target_properties(pengui_playground PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
if(NOT MSVC)
 target_compile_options(pengui_playground PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
# Run for every app build, including YAML/image-only changes that do not relink the executable.
add_custom_target(pengui_playground_resources
 COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:pengui_playground>"
 COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/resources/playground.yaml" "$<TARGET_FILE_DIR:pengui_playground>/playground.yaml"
 COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/resources/icons" "$<TARGET_FILE_DIR:pengui_playground>/icons"
 VERBATIM)
add_dependencies(pengui_playground pengui_playground_resources)

# Editor shell is a separate example app; pengUI itself has no editor dependency.
include(${PROJECT_SOURCE_DIR}/cmake/TextFonts.cmake)
add_library(pengui_editor_support STATIC
 platform/fonts.c
 ${PROJECT_SOURCE_DIR}/editors/vector/object/commands.c
 ${PROJECT_SOURCE_DIR}/editors/vector/text/text.c
 ${PROJECT_SOURCE_DIR}/editors/vector/text/layout.c
 ${PROJECT_SOURCE_DIR}/editors/vector/text/tool.c
 ${PROJECT_SOURCE_DIR}/editors/vector/text/render.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ui/inspector/text.c
 documents/controller.c
 ${PROJECT_SOURCE_DIR}/editors/vector/persistence/document_file.c
 ${PROJECT_SOURCE_DIR}/editors/vector/workspace/files.c
 project/project.c project/state.c project/explorer.c project/controller.c
 editor/config.c shell/shell.c platform/temp_file.c platform/files.c
 editor_host/host.c editor_host/checkpoint.c editor_host/registry.c editor_host/resources.c
 ${PROJECT_SOURCE_DIR}/editors/vector/adapter.c
 ${PROJECT_SOURCE_DIR}/editors/vector/editor.c
 ${PROJECT_SOURCE_DIR}/editors/vector/document/selection.c
 ${PROJECT_SOURCE_DIR}/editors/vector/rectangle/transform.c
 ${PROJECT_SOURCE_DIR}/editors/vector/document/document.c
 ${PROJECT_SOURCE_DIR}/editors/vector/persistence/checkpoint.c
 ${PROJECT_SOURCE_DIR}/editors/vector/persistence/commands.c
 ${PROJECT_SOURCE_DIR}/editors/vector/rectangle/rectangle.c
 ${PROJECT_SOURCE_DIR}/editors/vector/workspace/box_tool.c
 ${PROJECT_SOURCE_DIR}/editors/vector/rectangle/stroke_render.c
 ${PROJECT_SOURCE_DIR}/editors/vector/geometry/path.c
 ${PROJECT_SOURCE_DIR}/editors/vector/geometry/path_tess.c
 ${PROJECT_SOURCE_DIR}/editors/vector/object/shape.c
 ${PROJECT_SOURCE_DIR}/editors/vector/path/operations.c
 ${PROJECT_SOURCE_DIR}/editors/vector/path/render.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ui/context_menu/path_actions.c
 ${PROJECT_SOURCE_DIR}/editors/vector/object/orientation.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ui/context_menu/orientation_actions.c
 ${PROJECT_SOURCE_DIR}/editors/vector/object/alignment.c
 ${PROJECT_SOURCE_DIR}/editors/vector/object/order.c
 ${PROJECT_SOURCE_DIR}/editors/vector/object/group.c
 ${PROJECT_SOURCE_DIR}/editors/vector/object/clipboard.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ui/context_menu/object_actions.c
 ${PROJECT_SOURCE_DIR}/editors/vector/rectangle/render.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ui/inspector/shape.c
 ${PROJECT_SOURCE_DIR}/editors/vector/artboard/artboard.c
 ${PROJECT_SOURCE_DIR}/editors/vector/artboard/tool.c
 ${PROJECT_SOURCE_DIR}/editors/vector/workspace/marquee.c
 ${PROJECT_SOURCE_DIR}/editors/vector/workspace/selection.c
 ${PROJECT_SOURCE_DIR}/editors/vector/geometry/rotation.c
 ${PROJECT_SOURCE_DIR}/editors/vector/workspace/rotation.c
 ${PROJECT_SOURCE_DIR}/editors/vector/workspace/oriented_resize.c
 ${PROJECT_SOURCE_DIR}/editors/vector/geometry/constraints.c
 ${PROJECT_SOURCE_DIR}/editors/vector/workspace/modifiers.c
 ${PROJECT_SOURCE_DIR}/editors/vector/geometry/scale.c
 ${PROJECT_SOURCE_DIR}/editors/vector/workspace/group_transform.c
 ${PROJECT_SOURCE_DIR}/editors/vector/geometry/resize.c
 ${PROJECT_SOURCE_DIR}/editors/vector/workspace/ruler.c
 ${PROJECT_SOURCE_DIR}/editors/vector/history/history.c
 ${PROJECT_SOURCE_DIR}/editors/vector/workspace/canvas.c
 ${PROJECT_SOURCE_DIR}/editors/vector/workspace/documents.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ui/inspector/inspector.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ui/document_tree/groups.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ui/document_tree/document_tree.c
 ${PROJECT_SOURCE_DIR}/editors/vector/geometry/bezier.c
 ${PROJECT_SOURCE_DIR}/editors/vector/polygon/polygon.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ellipse/ellipse.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ellipse/render.c
 ${PROJECT_SOURCE_DIR}/editors/vector/curve/curve.c
 ${PROJECT_SOURCE_DIR}/editors/vector/curve/tool.c
 ${PROJECT_SOURCE_DIR}/editors/vector/curve/render.c
 ${PROJECT_SOURCE_DIR}/editors/vector/geometry/element_snap.c
 ${PROJECT_SOURCE_DIR}/editors/vector/workspace/element_snap.c
 ${PROJECT_SOURCE_DIR}/editors/vector/grid/snap.c
 ${PROJECT_SOURCE_DIR}/editors/vector/grid/render.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ui/tools/grid.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ui/toolbar/toolbar.c
 ${PROJECT_SOURCE_DIR}/editors/scripts/adapter.c
 ${PROJECT_SOURCE_DIR}/editors/scripts/editor.c
 ${PROJECT_SOURCE_DIR}/editors/scripts/inspector.c
 ${PROJECT_SOURCE_DIR}/editors/game/adapter.c
 ${PROJECT_SOURCE_DIR}/editors/game/editor.c
 ${PROJECT_SOURCE_DIR}/editors/game/inspector.c)
target_include_directories(pengui_editor_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR})
target_link_libraries(pengui_editor_support PUBLIC pengUI::sdl PRIVATE pengui_yaml_parser pengui_tess pengui_truetype)
add_executable(pengui_editor editor/main.c editor/application.c)
target_link_libraries(pengui_editor PRIVATE pengui_editor_support)
set_target_properties(pengui_editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
if(NOT MSVC)
 target_compile_options(pengui_editor_support PRIVATE -Wall -Wextra -Wpedantic -Werror)
 target_compile_options(pengui_editor PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_custom_target(pengui_editor_resources
 COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/resources/project-explorer.yaml" "$<TARGET_FILE_DIR:pengui_editor>/project-explorer.yaml"
 COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/resources/ui/fonts" "$<TARGET_FILE_DIR:pengui_editor>/fonts"
 COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:pengui_editor>"
 COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/resources/application.yaml" "$<TARGET_FILE_DIR:pengui_editor>/application.yaml"
 COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/resources/shell.yaml" "$<TARGET_FILE_DIR:pengui_editor>/shell.yaml"
 COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/resources/icons" "$<TARGET_FILE_DIR:pengui_editor>/icons"
 COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/editors/vector/ui" "$<TARGET_FILE_DIR:pengui_editor>/editors/vector/ui"
 COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/editors/scripts/ui" "$<TARGET_FILE_DIR:pengui_editor>/editors/scripts/ui"
 COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/editors/game/ui" "$<TARGET_FILE_DIR:pengui_editor>/editors/game/ui"
 VERBATIM)
add_dependencies(pengui_editor pengui_editor_resources)
