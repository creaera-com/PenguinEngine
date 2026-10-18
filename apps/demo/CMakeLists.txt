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
add_library(pengui_editor_support STATIC
 editor/config.c shell/shell.c
 editor_host/host.c editor_host/registry.c editor_host/resources.c
 ${PROJECT_SOURCE_DIR}/editors/vector/adapter.c
 ${PROJECT_SOURCE_DIR}/editors/vector/editor.c
 ${PROJECT_SOURCE_DIR}/editors/vector/document.c
 ${PROJECT_SOURCE_DIR}/editors/vector/artboard.c
 ${PROJECT_SOURCE_DIR}/editors/vector/artboard_tool.c
 ${PROJECT_SOURCE_DIR}/editors/vector/ruler.c
 ${PROJECT_SOURCE_DIR}/editors/vector/history.c
 ${PROJECT_SOURCE_DIR}/editors/vector/canvas.c
 ${PROJECT_SOURCE_DIR}/editors/vector/inspector.c
 ${PROJECT_SOURCE_DIR}/editors/scripts/adapter.c
 ${PROJECT_SOURCE_DIR}/editors/scripts/editor.c
 ${PROJECT_SOURCE_DIR}/editors/scripts/inspector.c
 ${PROJECT_SOURCE_DIR}/editors/game/adapter.c
 ${PROJECT_SOURCE_DIR}/editors/game/editor.c
 ${PROJECT_SOURCE_DIR}/editors/game/inspector.c)
target_include_directories(pengui_editor_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR})
target_link_libraries(pengui_editor_support PUBLIC pengUI::sdl PRIVATE pengui_yaml_parser)
add_executable(pengui_editor editor/main.c editor/application.c)
target_link_libraries(pengui_editor PRIVATE pengui_editor_support)
set_target_properties(pengui_editor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
if(NOT MSVC)
 target_compile_options(pengui_editor_support PRIVATE -Wall -Wextra -Wpedantic -Werror)
 target_compile_options(pengui_editor PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_custom_target(pengui_editor_resources
 COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:pengui_editor>"
 COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/resources/application.yaml" "$<TARGET_FILE_DIR:pengui_editor>/application.yaml"
 COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/resources/shell.yaml" "$<TARGET_FILE_DIR:pengui_editor>/shell.yaml"
 COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/resources/icons" "$<TARGET_FILE_DIR:pengui_editor>/icons"
 COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/editors/vector/ui" "$<TARGET_FILE_DIR:pengui_editor>/editors/vector/ui"
 COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/editors/scripts/ui" "$<TARGET_FILE_DIR:pengui_editor>/editors/scripts/ui"
 COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/editors/game/ui" "$<TARGET_FILE_DIR:pengui_editor>/editors/game/ui"
 VERBATIM)
add_dependencies(pengui_editor pengui_editor_resources)
