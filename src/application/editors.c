#include "application/internal.h"
#include "editors/game/map/adapter.h"
#include "editors/graphics/vector/adapter.h"
#include "editors/script/adapter.h"

const EditorView *const editor_views[] = {&pe_map_view, &pe_script_view, &pe_vector_view};
const int editor_view_count = (int)(sizeof editor_views / sizeof editor_views[0]);

_Static_assert(sizeof editor_views / sizeof editor_views[0] <= UI_MAX_TABS,
               "Editor registry exceeds workspace session capacity");

const PeEditorRegistry editor_registry = {editor_views,
                                          sizeof editor_views / sizeof editor_views[0]};
