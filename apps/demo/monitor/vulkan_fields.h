/** @file
 * @brief Versioned property names shared by monitor sampling and JSON serialization. */
#ifndef PENGUENGINE_MONITOR_VULKAN_FIELDS_H
#define PENGUENGINE_MONITOR_VULKAN_FIELDS_H
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
#endif
