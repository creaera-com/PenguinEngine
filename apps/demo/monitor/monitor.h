/** @file
 * @brief Shared host-owned process monitor, independent of any concrete editor. */
#ifndef PENGUENGINE_PROCESS_MONITOR_H
#define PENGUENGINE_PROCESS_MONITOR_H
#include "editor_host/editor.h"
#include "history.h"
#include "log.h"
/** @brief Observe the shared renderer and effect texture capacity; never reads GPU pixels.
 * Only value copies are retained. Call while drawing, before the next monitor tick.
 */
void process_monitor_graphics(ProcessMonitor *monitor, SDL_Renderer *renderer);
/** @brief Sample at 500 ms intervals and update the shell status label when data changes. */
void process_monitor_tick(ProcessMonitor *monitor, PUIView *view, double now);
/** @brief Compose monitor.yaml into optional process_monitor_content; caller owns a returned copy.
 * Returns NULL without an error when the layout has no monitor slot. */
PUILayout *process_monitor_layout(const EditorServices *services, const PUILayout *base,
                                  bool *requested, char *error, size_t capacity);
/** @brief Draw shared CPU/RAM/GPU-effect-memory canvas IDs; false delegates unrelated canvases to
 * the editor. */
bool process_monitor_draw(const ProcessMonitor *monitor, SDL_Renderer *renderer, PUIRect area,
                          const char *widget);
/** @brief Reflect recording state/errors in optional YAML controls after UI composition. */
void process_monitor_recording_sync(const MonitorLog *log, PUIView *view);
/** @brief Handle the monitor.log toggle; requires the host's current project and context. */
bool process_monitor_recording_action(MonitorLog *log, const ProcessMonitor *monitor, PUIView *view,
                                      const char *editor, EditorDiagnostics context, double now);
/** @brief Optionally flush and wait after SDL_RenderPresent; false disables a failed request. */
bool process_monitor_gpu_wait(ProcessMonitor *monitor, SDL_Renderer *renderer);
/** @brief Reflect requested versus actually available GPU waiting in YAML controls. */
void process_monitor_gpu_wait_sync(const ProcessMonitor *monitor, PUIView *view);
#endif
