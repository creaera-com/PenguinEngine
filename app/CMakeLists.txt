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
