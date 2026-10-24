/** @file
 * @brief Versioned property names shared by monitor sampling and JSON serialization. */
#ifndef PENGUENGINE_MONITOR_VULKAN_FIELDS_H
#define PENGUENGINE_MONITOR_VULKAN_FIELDS_H
#include "history.h"
static const char *const monitor_vulkan_fields[] = {
    "descriptor_pools_created",
    "descriptor_pools_destroyed",
    "descriptor_sets_allocated",
    "descriptor_sets_fetched",
    "descriptor_caches_returned",
    "descriptor_caches_reused",
    "command_buffers_allocated",
    "command_buffers_acquired",
    "command_buffers_freed",
    "memory_allocations",
    "memory_frees",
    "memory_bytes_allocated",
    "memory_bytes_freed",
    "memory_maps",
    "memory_implicit_unmaps",
    "failures",
};
static const char *const monitor_vulkan_classes[] = {"gpu_buffer", "uniform_buffer",
                                                     "transfer_buffer", "image"};
static const char *const monitor_vulkan_class_fields[] = {
    "allocations", "frees",          "allocated_bytes",   "freed_bytes",
    "bindings",    "resource_bytes", "requirement_bytes", "dedicated_allocations"};
static const char *const monitor_vulkan_type_fields[] = {"flags", "heap_index",      "allocations",
                                                         "frees", "allocated_bytes", "freed_bytes",
                                                         "maps",  "implicit_unmaps"};
_Static_assert(sizeof monitor_vulkan_fields / sizeof *monitor_vulkan_fields ==
                   MONITOR_VULKAN_COUNTER_COUNT,
               "Vulkan counter catalog mismatch");
_Static_assert(sizeof monitor_vulkan_classes / sizeof *monitor_vulkan_classes ==
                   MONITOR_VULKAN_CLASS_COUNT,
               "Vulkan class catalog mismatch");
_Static_assert(sizeof monitor_vulkan_class_fields / sizeof *monitor_vulkan_class_fields ==
                   MONITOR_VULKAN_DETAIL_COUNT,
               "Vulkan class field catalog mismatch");
_Static_assert(sizeof monitor_vulkan_type_fields / sizeof *monitor_vulkan_type_fields ==
                   MONITOR_VULKAN_DETAIL_COUNT,
               "Vulkan type field catalog mismatch");
#endif
