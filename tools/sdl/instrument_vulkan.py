#!/usr/bin/env python3
"""Instrument exactly SDL 3.4.18 in a private build copy; fail closed on drift."""
from pathlib import Path
import sys


def instrument(text):
    if 'PENGUI_VULKAN_DIAGNOSTICS_V2' in text:
        return text

    if 'PENGUI_VULKAN_DIAGNOSTICS_V1' in text:
        raise ValueError('Reconfigure from clean SDL source; old diagnostic patch detected')

    def replace(old, new):
        nonlocal text
        if text.count(old) != 1:
            raise ValueError(f'SDL Vulkan source mismatch: {old!r}')
        text = text.replace(old, new)

    def count(key, value='1'):
        return f'    PUI_VulkanCount(renderer, "{key}", {value});\n'

    replace('// Forward declarations\n', '''/* PENGUI_VULKAN_DIAGNOSTICS_V2: counters only; no pool policy changes. */
static void PUI_VulkanCount(VulkanRenderer *renderer, const char *name, Sint64 amount)
{
    char key[128];
    SDL_snprintf(key, sizeof(key), "penguengine.vulkan.%s", name);
    SDL_LockProperties(renderer->props);
    SDL_SetNumberProperty(renderer->props, key,
        SDL_GetNumberProperty(renderer->props, key, 0) + amount);
    SDL_UnlockProperties(renderer->props);
}

static void PUI_VulkanTypeCount(VulkanRenderer *renderer, Uint32 index, const char *name, Sint64 amount)
{
    char key[96];
    SDL_snprintf(key, sizeof(key), "memory_type.%u.%s", index, name);
    PUI_VulkanCount(renderer, key, amount);
}

static void PUI_VulkanClassCount(VulkanRenderer *renderer, Uint32 kind, const char *name, Sint64 amount)
{
    char key[96];
    SDL_snprintf(key, sizeof(key), "class.%u.%s", kind, name);
    PUI_VulkanCount(renderer, key, amount);
}

static void PUI_VulkanAllocation(VulkanRenderer *renderer, VulkanMemoryAllocation *allocation, bool created)
{
    Uint32 index = allocation->allocator->memoryTypeIndex;
    const char *count = created ? "allocations" : "frees";
    const char *bytes = created ? "allocated_bytes" : "freed_bytes";
    PUI_VulkanClassCount(renderer, allocation->puiDiagnosticKind, count, 1);
    PUI_VulkanClassCount(renderer, allocation->puiDiagnosticKind, bytes, (Sint64)allocation->size);
    PUI_VulkanTypeCount(renderer, index, count, 1);
    PUI_VulkanTypeCount(renderer, index, bytes, (Sint64)allocation->size);
}

static void PUI_VulkanBinding(VulkanRenderer *renderer, Uint32 kind, VkDeviceSize logical, VkDeviceSize required)
{
    PUI_VulkanClassCount(renderer, kind, "bindings", 1);
    PUI_VulkanClassCount(renderer, kind, "resource_bytes", (Sint64)logical);
    PUI_VulkanClassCount(renderer, kind, "requirement_bytes", (Sint64)required);
}

// Forward declarations
''')
    replace('    renderer->props = SDL_CreateProperties();',
            '    renderer->props = SDL_CreateProperties();\n    SDL_SetBooleanProperty(renderer->props, "penguengine.vulkan.available", true);\n    SDL_SetNumberProperty(renderer->props, "penguengine.vulkan.version", 2);\n    SDL_SetNumberProperty(renderer->props, "penguengine.vulkan.memory_type_count", renderer->memoryProperties.memoryTypeCount);\n    for (Uint32 puiIndex = 0; puiIndex < renderer->memoryProperties.memoryTypeCount; puiIndex += 1) {\n        PUI_VulkanTypeCount(renderer, puiIndex, "flags", renderer->memoryProperties.memoryTypes[puiIndex].propertyFlags);\n        PUI_VulkanTypeCount(renderer, puiIndex, "heap_index", renderer->memoryProperties.memoryTypes[puiIndex].heapIndex);\n    }')
    for api, key, amount in [
        ('vkCreateDescriptorPool', 'descriptor_pools_created', '1'),
        ('vkAllocateDescriptorSets', 'descriptor_sets_allocated', 'descriptorSetCount'),
        ('vkAllocateCommandBuffers', 'command_buffers_allocated', 'allocateInfo.commandBufferCount'),
    ]:
        marker = f'    CHECK_VULKAN_ERROR_AND_RETURN(vulkanResult, {api}, false);'
        replace(marker, count('failures', '(vulkanResult != VK_SUCCESS)') + marker + '\n' + count(key, amount))
    replace('            renderer->vkDestroyDescriptorPool(',
            count('descriptor_pools_destroyed') + '            renderer->vkDestroyDescriptorPool(')
    replace('    pool->descriptorSetIndex += 1;',
            '    pool->descriptorSetIndex += 1;\n' + count('descriptor_sets_fetched'))
    replace('    renderer->descriptorSetCachePoolCount += 1;',
            '    renderer->descriptorSetCachePoolCount += 1;\n' + count('descriptor_caches_returned'))
    replace('        cache = renderer->descriptorSetCachePool[renderer->descriptorSetCachePoolCount - 1];',
            count('descriptor_caches_reused') + '        cache = renderer->descriptorSetCachePool[renderer->descriptorSetCachePoolCount - 1];')
    replace('    // Persistent mapping for host-visible memory',
            count('memory_allocations') + count('memory_bytes_allocated', '(Sint64)allocationSize') + '    PUI_VulkanAllocation(renderer, allocation, true);\n' + '\n    // Persistent mapping for host-visible memory')
    replace('        // Uh oh, we couldn\'t allocate, time to clean up',
            count('failures') + '        // Uh oh, we couldn\'t allocate, time to clean up')
    marker = '        CHECK_VULKAN_ERROR_AND_RETURN(result, vkMapMemory, 0);'
    replace(marker, count('failures', '(result != VK_SUCCESS)') + marker + '\n' + count('memory_maps') + '        PUI_VulkanTypeCount(renderer, memoryTypeIndex, \"maps\", 1);\n')
    replace('    renderer->vkFreeMemory(', '    PUI_VulkanAllocation(renderer, allocation, false);\n    PUI_VulkanTypeCount(renderer, allocation->allocator->memoryTypeIndex, \"implicit_unmaps\", allocation->mapPointer != NULL);\n' + count('memory_frees') + count('memory_bytes_freed', '(Sint64)allocation->size') +
            '    /* vkFreeMemory implicitly unmaps persistent host mappings. */\n' +
            count('memory_implicit_unmaps', '(allocation->mapPointer != NULL)') + '    renderer->vkFreeMemory(')
    replace('    commandPool->inactiveCommandBufferCount -= 1;',
            '    commandPool->inactiveCommandBufferCount -= 1;\n' + count('command_buffers_acquired'))
    replace('    renderer->vkDestroyCommandPool(',
            count('command_buffers_freed', 'commandPool->inactiveCommandBufferCount') + '    renderer->vkDestroyCommandPool(')
    # Record the resource that triggered a backing allocation. A shared memory
    # allocation may subsequently serve other classes; these are not ownership tags.
    replace('    VulkanMemorySubAllocator *allocator;\n    VkDeviceMemory memory;',
            '    VulkanMemorySubAllocator *allocator;\n    Uint32 puiDiagnosticKind;\n    VkDeviceMemory memory;')
    replace('    VulkanMemoryAllocation **pMemoryAllocation)\n',
            '    VulkanMemoryAllocation **pMemoryAllocation,\n    Uint32 puiDiagnosticKind)\n')
    replace('    allocation->size = allocationSize;',
            '    allocation->size = allocationSize;\n    allocation->puiDiagnosticKind = puiDiagnosticKind;')
    replace('    VulkanMemoryUsedRegion **pMemoryUsedRegion)\n',
            '    VulkanMemoryUsedRegion **pMemoryUsedRegion,\n    Uint32 puiDiagnosticKind)\n')
    replace('        isHostVisible,\n        &allocation);',
            '        isHostVisible,\n        &allocation,\n        puiDiagnosticKind);')
    replace('            image,\n            usedRegion);',
            '            image,\n            usedRegion,\n            3);')
    replace('            VK_NULL_HANDLE,\n            usedRegion);',
            '            VK_NULL_HANDLE,\n            usedRegion,\n            (Uint32)type);')
    # Two success exits: reuse of a suballocation and creation of new backing memory.
    for indent in ('            ', '    '):
        marker = indent + '*pMemoryUsedRegion = usedRegion;\n' + indent + 'return 1;'
        replace(marker, indent + 'PUI_VulkanBinding(renderer, puiDiagnosticKind, resourceSize, requiredSize);\n' + marker)
    marker = '    usedRegion = VULKAN_INTERNAL_NewMemoryUsedRegion(\n        renderer,\n        allocation,\n        0,'
    replace(marker, '    if (dedicated) {\n        PUI_VulkanClassCount(renderer, puiDiagnosticKind, "dedicated_allocations", 1);\n    }\n\n' + marker)
    return text


if __name__ == '__main__':
    path = Path(sys.argv[1]) / 'src/gpu/vulkan/SDL_gpu_vulkan.c'
    original = path.read_text()
    patched = instrument(original)
    if patched != original:
        path.write_text(patched)
