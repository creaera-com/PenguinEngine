/** @file
 * @brief Native counters for this process only; unsupported readings remain unavailable. */
#ifndef PENGUENGINE_PROCESS_METRICS_H
#define PENGUENGINE_PROCESS_METRICS_H
#include "monitor/history.h"
/** @brief Read current resident bytes and cumulative user+kernel CPU seconds for all threads. */
ProcessCounters process_metrics_read(void);
#endif
