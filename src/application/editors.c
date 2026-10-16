#include "application/internal.h"
#include "editors/game/map/adapter.h"
#include "editors/graphics/vector/adapter.h"
#include "editors/script/adapter.h"

const EditorView editor_views[] = {{UI_MAP_VIEW, "Map", ICON_GRID, 9, editor_draw_map_tools,
                                    editor_draw_map_content, &pe_map_backend},
                                   {UI_SCRIPT_VIEW, "Scripts", ICON_FILE, 1,
                                    editor_draw_script_tools, editor_draw_script_content,
                                    &pe_script_backend},
                                   {UI_VECTOR_VIEW, "Vector Art", ICON_EDIT, 1, vector_draw_tools,
                                    vector_draw_canvas, &pe_vector_backend}};
const int editor_view_count = (int)(sizeof editor_views / sizeof editor_views[0]);
