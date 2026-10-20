# Interaction fixtures use 1280x800 and stable pane widths, independent of user preferences.
# Keep the complete current UI (IDs, controls and theme); normalize only the five outer widths.
foreach(resource shell.yaml application.yaml project-explorer.yaml test-editor-config.yaml test-invalid-editor-config.yaml fonts icons editors)
 file(COPY "${SOURCE_ROOT}/${resource}" DESTINATION "${DESTINATION}")
endforeach()
set(layout_path "${DESTINATION}/editors/vector/ui/editor.yaml")
file(READ "${layout_path}" layout)
string(FIND "${layout}" "  children:" boundary)
if(boundary LESS 0)
 message(FATAL_ERROR "Vector interaction fixture requires a root split")
endif()
string(SUBSTRING "${layout}" 0 ${boundary} header)
string(SUBSTRING "${layout}" ${boundary} -1 children)
string(REGEX REPLACE "\n  (initial|initial_last|minimum_first|minimum_second|minimum_last): [0-9.]+"
 "\n  \\1: 250" header "${header}")
file(WRITE "${layout_path}" "${header}${children}")
