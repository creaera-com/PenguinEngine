#!/usr/bin/env python3
"""Instrument exactly SDL 3.4.18 in a private build copy; fail closed on drift."""
from pathlib import Path
import sys


def instrument(text):
    if 'PENGUI_VULKAN_DIAGNOSTICS_V1' in text:
        return text

    def replace(old, new):
        nonlocal text
        if text.count(old) != 1:
            raise ValueError(f'SDL Vulkan source mismatch: {old!r}')
        text = text.replace(old, new)

    def count(key, value='1'):
        return f'    PUI_VulkanCount(renderer, "{key}", {value});\n'

    replace('// Forward declarations\n', '''/* PENGUI_VULKAN_DIAGNOSTICS_V1: counters only; no pool policy changes. */
static void PUI_VulkanCount(VulkanRenderer *renderer, const char *name, Sint64 amount)
{
    char key[128];
    SDL_snprintf(key, sizeof(key), "penguengine.vulkan.%s", name);
    SDL_LockProperties(renderer->props);
    SDL_SetNumberProperty(renderer->props, key,
        SDL_GetNumberProperty(renderer->props, key, 0) + amount);
    SDL_UnlockProperties(renderer->props);
}

// Forward declarations
''')
    replace('    renderer->props = SDL_CreateProperties();',
            '    renderer->props = SDL_CreateProperties();\n    SDL_SetBooleanProperty(renderer->props, "penguengine.vulkan.available", true);')
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
            count('memory_allocations') + count('memory_bytes_allocated', '(Sint64)allocationSize') + '\n    // Persistent mapping for host-visible memory')
    replace('        // Uh oh, we couldn\'t allocate, time to clean up',
            count('failures') + '        // Uh oh, we couldn\'t allocate, time to clean up')
    marker = '        CHECK_VULKAN_ERROR_AND_RETURN(result, vkMapMemory, 0);'
    replace(marker, count('failures', '(result != VK_SUCCESS)') + marker + '\n' + count('memory_maps'))
    replace('    renderer->vkFreeMemory(', count('memory_frees') + count('memory_bytes_freed', '(Sint64)allocation->size') +
            '    /* vkFreeMemory implicitly unmaps persistent host mappings. */\n' +
            count('memory_implicit_unmaps', '(allocation->mapPointer != NULL)') + '    renderer->vkFreeMemory(')
    replace('    commandPool->inactiveCommandBufferCount -= 1;',
            '    commandPool->inactiveCommandBufferCount -= 1;\n' + count('command_buffers_acquired'))
    replace('    renderer->vkDestroyCommandPool(',
            count('command_buffers_freed', 'commandPool->inactiveCommandBufferCount') + '    renderer->vkDestroyCommandPool(')
    return text


if __name__ == '__main__':
    path = Path(sys.argv[1]) / 'src/gpu/vulkan/SDL_gpu_vulkan.c'
    original = path.read_text()
    patched = instrument(original)
    if patched != original:
        path.write_text(patched)
