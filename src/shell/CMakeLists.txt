if(PENGUIN_BUILD_EDITOR)
 target_sources(penguin_editor PRIVATE
  console/view.c
  input.c
  shell.c
  widgets.c
  explorer/file_tree.c
  dialogs/fields.c)
endif()
