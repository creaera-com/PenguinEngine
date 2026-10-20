/** @file @brief Render-only expansion using current live source geometry and styles. */
#ifndef PE_VECTOR_REPEAT_RENDER_H
#define PE_VECTOR_REPEAT_RENDER_H
#include "editors/vector/document/document.h"
/** @brief Paint non-original cells synchronously; instance values are borrowed only for the call.
 */
void vector_repeat_render(const VectorDocument *document, const VectorShape *source,
                          void (*paint)(void *, VectorShape *), void *context);
#endif
