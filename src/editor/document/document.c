#include "editor/document/document.h"

PeDocumentStatus pe_document_status(PeDocument document) {
    if (!document.context || !document.ops || !document.ops->status) {
        return (PeDocumentStatus){0};
    }
    return document.ops->status(document.context);
}

bool pe_document_save(PeDocument document) {
    PeDocumentStatus status = pe_document_status(document);
    return status.loaded && !status.pending && document.ops->save &&
           document.ops->save(document.context);
}

void pe_document_undo(PeDocument document, bool redo) {
    PeDocumentStatus status = pe_document_status(document);
    if (status.loaded && !status.pending && (redo ? status.can_redo : status.can_undo) &&
        document.ops->undo) {
        document.ops->undo(document.context, redo);
    }
}

void pe_document_describe(PeDocument document, char *buffer, size_t size) {
    if (!buffer || !size) {
        return;
    }
    buffer[0] = 0;
    if (document.context && document.ops && document.ops->describe) {
        document.ops->describe(document.context, buffer, size);
    }
    buffer[size - 1] = 0;
}
