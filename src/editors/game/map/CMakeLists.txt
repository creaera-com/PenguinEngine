if(PENGUIN_BUILD_EDITOR)
 target_sources(penguin_editor PRIVATE
  adapter.c
  actions.c
  fields.c
  runtime.c
  input.c
  document.c
  tools.c
  panels.c
  map_panel.c
  tiles_panel.c
  components.c
  scene_tree.c
  canvas.c)
endif()
