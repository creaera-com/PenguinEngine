#include "platform/terminal/process.h"
#include <stdio.h>

struct PeTerminalProcess {
    int unused;
};

PeTerminalProcess *pe_terminal_process_start(const char *root, const char *shell, int rows,
                                             int cols, char *error, size_t size) {
    (void)root;
    (void)shell;
    (void)rows;
    (void)cols;
    snprintf(error, size,
             "Terminal backend unavailable: Windows ConPTY adapter is not implemented yet");
    return NULL;
}

int pe_terminal_process_read(PeTerminalProcess *p, char *buffer, size_t size) {
    (void)p;
    (void)buffer;
    (void)size;
    return -1;
}

int pe_terminal_process_write(PeTerminalProcess *p, const char *buffer, size_t size) {
    (void)p;
    (void)buffer;
    (void)size;
    return -1;
}

bool pe_terminal_process_resize(PeTerminalProcess *p, int rows, int cols) {
    (void)p;
    (void)rows;
    (void)cols;
    return false;
}

bool pe_terminal_process_running(PeTerminalProcess *p) {
    (void)p;
    return false;
}

void pe_terminal_process_free(PeTerminalProcess *p) {
    (void)p;
}
