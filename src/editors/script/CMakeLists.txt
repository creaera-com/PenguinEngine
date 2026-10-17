if(PENGUIN_BUILD_EDITOR)
 target_sources(penguin_editor PRIVATE
  adapter.c
  script_editor.c
  actions.c
  panels.c)
endif()
