#!/usr/bin/env python3
"""Bound SDL 3.4.18 GPU upload cycling by payload size, not historical maximum."""
from pathlib import Path
import hashlib
import sys


def patch(text):
    if 'PENGUI_GPU_UPLOAD_SIZE_CLASSES_V1' in text:
        return text
    if hashlib.sha256(text.encode()).hexdigest() != '5ff5de62c6d0f1b6d58fdd76454ad2adc8118e2ac9e7b1717e159524d5062284':
        raise ValueError('Expected pristine SDL 3.4.18 GPU renderer source')

    def replace(old, new):
        nonlocal text
        if text.count(old) != 1:
            raise ValueError(f'SDL GPU upload source mismatch: {old!r}')
        text = text.replace(old, new)

    marker = '    struct\n    {\n        SDL_GPUTransferBuffer *transfer_buf;\n        SDL_GPUBuffer *buffer;\n        Uint32 buffer_size;\n    } vertices;'
    replace(marker, '''    /* PENGUI_GPU_UPLOAD_SIZE_CLASSES_V1: active upload aliases one owned slot. */
    struct PUI_VertexUpload
    {
        SDL_GPUTransferBuffer *transfer_buf;
        SDL_GPUBuffer *buffer;
        Uint32 buffer_size;
    } vertices, vertex_uploads[25];''')
    start = text.index('static void ReleaseVertexBuffer(')
    end = text.index('static bool UploadVertices(', start)
    original = text[start:end]
    if original.count('static bool InitVertexBuffer(') != 1:
        raise ValueError('SDL GPU vertex upload helpers changed')
    replace(original, '''static void ReleaseVertexBuffer(GPU_RenderData *data)
{
    for (unsigned i = 0; i < SDL_arraysize(data->vertex_uploads); i++) {
        if (data->vertex_uploads[i].buffer) {
            SDL_ReleaseGPUBuffer(data->device, data->vertex_uploads[i].buffer);
        }
        if (data->vertex_uploads[i].transfer_buf) {
            SDL_ReleaseGPUTransferBuffer(data->device, data->vertex_uploads[i].transfer_buf);
        }
        SDL_zero(data->vertex_uploads[i]);
    }
    SDL_zero(data->vertices);
}

static bool InitVertexBuffer(GPU_RenderData *data, Uint32 size)
{
    /* A large artwork batch must not determine the allocation size of every
     * subsequent tiny effect pass. Retain one independently cycled pair per
     * power-of-two size class. The last class handles UINT32_MAX without overflow. */
    Uint32 capacity = 256;
    unsigned bucket = 0;
    while (capacity < size) {
        capacity = capacity > SDL_MAX_UINT32 / 2 ? SDL_MAX_UINT32 : capacity * 2;
        bucket += 1;
    }

    if (data->vertex_uploads[bucket].buffer) {
        data->vertices = data->vertex_uploads[bucket];
        return true;
    }

    struct PUI_VertexUpload upload = { 0 };
    SDL_GPUBufferCreateInfo bci;
    SDL_zero(bci);
    bci.size = capacity;
    bci.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    upload.buffer = SDL_CreateGPUBuffer(data->device, &bci);
    if (!upload.buffer) {
        return false;
    }

    SDL_GPUTransferBufferCreateInfo tbci;
    SDL_zero(tbci);
    tbci.size = capacity;
    tbci.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    upload.transfer_buf = SDL_CreateGPUTransferBuffer(data->device, &tbci);
    if (!upload.transfer_buf) {
        SDL_ReleaseGPUBuffer(data->device, upload.buffer);
        return false;
    }

    upload.buffer_size = capacity;
    data->vertex_uploads[bucket] = upload;
    data->vertices = upload;
    return true;
}

''')
    replace('''    if (vertsize > data->vertices.buffer_size) {
        ReleaseVertexBuffer(data);
        if (!InitVertexBuffer(data, (Uint32)vertsize)) {
            return false;
        }
    }
''', '''    if (vertsize > SDL_MAX_UINT32) {
        return SDL_SetError("GPU vertex upload exceeds 32-bit buffer size");
    }
    if (!InitVertexBuffer(data, (Uint32)vertsize)) {
        return false;
    }
''')
    # SDL_MapGPUTransferBuffer may fail; do not memcpy through NULL on allocation failure.
    replace('    SDL_memcpy(staging_buf, vertices, vertsize);',
            '    if (!staging_buf) {\n        return false;\n    }\n    SDL_memcpy(staging_buf, vertices, vertsize);')
    return text


if __name__ == '__main__':
    path = Path(sys.argv[1]) / 'src/render/gpu/SDL_render_gpu.c'
    original = path.read_text()
    patched = patch(original)
    if patched != original:
        path.write_text(patched)
