#include "console/terminal.h"
#include "platform/terminal/process.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <time.h>
#endif
static PeTerminal *terminal;
static PeTerminalProcess *process;

void setUp(void) {
    terminal = pe_terminal_new(4, 20);
    TEST_ASSERT_NOT_NULL(terminal);
}

void tearDown(void) {
    pe_terminal_process_free(process);
    process = NULL;
    pe_terminal_free(terminal);
    terminal = NULL;
}

static void feed(const char *text) {
    pe_terminal_feed(terminal, text, strlen(text));
}

static void test_colors_cursor_and_carriage_return(void) {
    feed("hello\r\033[31mOK\033[0m");
    PeTerminalCell cell;
    TEST_ASSERT_TRUE(pe_terminal_cell(terminal, 0, 0, &cell));
    TEST_ASSERT_EQUAL_UINT32('O', cell.chars[0]);
    TEST_ASSERT_NOT_EQUAL(0xe9e9e9ff, cell.fg);
    pe_terminal_cell(terminal, 0, 2, &cell);
    TEST_ASSERT_EQUAL_UINT32('l', cell.chars[0]);
    int row, col;
    TEST_ASSERT_TRUE(pe_terminal_cursor(terminal, &row, &col));
    TEST_ASSERT_EQUAL_INT(0, row);
    TEST_ASSERT_EQUAL_INT(2, col);
    feed("\033[?25l");
    TEST_ASSERT_FALSE(pe_terminal_cursor(terminal, &row, &col));
}

static void test_utf8_and_alternate_screen(void) {
    pe_terminal_feed(terminal, "\xc3", 1);
    pe_terminal_feed(terminal, "\xa4", 1);
    PeTerminalCell cell;
    pe_terminal_cell(terminal, 0, 0, &cell);
    TEST_ASSERT_EQUAL_UINT32(0xe4, cell.chars[0]);
    feed("\033[?1049hother\033[?1049l");
    pe_terminal_cell(terminal, 0, 0, &cell);
    TEST_ASSERT_EQUAL_UINT32(0xe4, cell.chars[0]);
    char *copy = pe_terminal_copy(terminal, 0, 0, 0, 0);
    TEST_ASSERT_NOT_NULL(copy);
    TEST_ASSERT_EQUAL_STRING("\xc3\xa4", copy);
    free(copy);
    feed("\033[2J\033[H\xe7\x95\x8cx");
    pe_terminal_cell(terminal, 0, 0, &cell);
    TEST_ASSERT_EQUAL_INT(2, cell.width);
    pe_terminal_cell(terminal, 0, 1, &cell);
    TEST_ASSERT_EQUAL_INT(0, cell.width);
    copy = pe_terminal_copy(terminal, 0, 0, 0, 2);
    TEST_ASSERT_NOT_NULL(copy);
    TEST_ASSERT_EQUAL_STRING("\xe7\x95\x8cx", copy);
    free(copy);
}

static void test_scrollback_resize_and_copy(void) {
    for (int i = 0; i < PE_TERMINAL_HISTORY + 20; i++) {
        feed("row\r\n");
    }
    TEST_ASSERT_EQUAL_INT(PE_TERMINAL_HISTORY, pe_terminal_history(terminal));
    pe_terminal_scroll(terminal, 99999);
    char *copy = pe_terminal_copy(terminal, 0, 0, 1, 5);
    TEST_ASSERT_NOT_NULL(copy);
    TEST_ASSERT_EQUAL_STRING("row\nrow", copy);
    free(copy);
    TEST_ASSERT_TRUE(pe_terminal_resize(terminal, 6, 30));
    TEST_ASSERT_EQUAL_INT(6, pe_terminal_rows(terminal));
    TEST_ASSERT_EQUAL_INT(30, pe_terminal_cols(terminal));
    TEST_ASSERT_FALSE(pe_terminal_resize(terminal, 1000, 1000));
    feed("\033[3J");
    TEST_ASSERT_EQUAL_INT(0, pe_terminal_history(terminal));
}

static void test_keyboard_and_partial_output(void) {
    pe_terminal_unichar(terminal, 'c', 4);
    pe_terminal_key(terminal, PE_TERM_UP, 0);
    size_t n;
    const char *bytes = pe_terminal_output(terminal, &n);
    TEST_ASSERT_EQUAL_UINT(4, n);
    TEST_ASSERT_EQUAL_MEMORY("\x03\033[A", bytes, 4);
    pe_terminal_consume(terminal, 1);
    bytes = pe_terminal_output(terminal, &n);
    TEST_ASSERT_EQUAL_UINT(3, n);
    TEST_ASSERT_EQUAL_MEMORY("\033[A", bytes, 3);
    pe_terminal_consume(terminal, 99);
    pe_terminal_output(terminal, &n);
    TEST_ASSERT_EQUAL_UINT(0, n);
}

static void test_bracketed_paste_and_limit(void) {
    feed("\033[?2004h");
    TEST_ASSERT_TRUE(pe_terminal_paste(terminal, "abc", true));
    size_t n;
    const char *bytes = pe_terminal_output(terminal, &n);
    TEST_ASSERT_EQUAL_UINT(15, n);
    TEST_ASSERT_EQUAL_MEMORY("\033[200~abc\033[201~", bytes, n);
    char *large = malloc(PE_TERMINAL_OUTPUT + 1);
    TEST_ASSERT_NOT_NULL(large);
    memset(large, 'x', PE_TERMINAL_OUTPUT);
    large[PE_TERMINAL_OUTPUT] = 0;
    TEST_ASSERT_FALSE(pe_terminal_paste(terminal, large, true));
    free(large);
    pe_terminal_output(terminal, &n);
    TEST_ASSERT_EQUAL_UINT(15, n);
}
#ifndef _WIN32
static bool await_text(const char *expected) {
    char text[16384] = {0};
    size_t used = 0;
    for (int i = 0; i < 400; i++) {
        int n = pe_terminal_process_read(process, text + used, sizeof text - used - 1);
        if (n > 0) {
            used += (size_t)n;
            text[used] = 0;
            if (strstr(text, expected)) {
                return true;
            }
        }
        struct timespec delay = {0, 5000000};
        nanosleep(&delay, NULL);
    }
    return false;
}

static bool send_text(const char *text) {
    size_t n = strlen(text), offset = 0;
    for (int i = 0; i < 400 && offset < n; i++) {
        int written = pe_terminal_process_write(process, text + offset, n - offset);
        if (written < 0) {
            return false;
        }
        offset += (size_t)written;
        if (!written) {
            struct timespec delay = {0, 5000000};
            nanosleep(&delay, NULL);
        }
    }
    return offset == n;
}

static void test_real_pty_commands_resize_interrupt_and_exit(void) {
    char error[256];
    process = pe_terminal_process_start(".", "/bin/sh", 8, 80, error, sizeof error);
    TEST_ASSERT_NOT_NULL_MESSAGE(process, error);
    TEST_ASSERT_TRUE(send_text("test -t 0 && printf '__%s__\\n' PTY_READY\n"));
    TEST_ASSERT_TRUE(await_text("__PTY_READY__"));
    TEST_ASSERT_TRUE(send_text("cd /; printf '__%s__\\n' \"$PWD\"\n"));
    TEST_ASSERT_TRUE(await_text("__/__"));
    TEST_ASSERT_TRUE(pe_terminal_process_resize(process, 11, 91));
    TEST_ASSERT_TRUE(send_text("stty size\n"));
    TEST_ASSERT_TRUE(await_text("11 91"));
    TEST_ASSERT_TRUE(send_text("sleep 30\n"));
    struct timespec delay = {0, 100000000};
    nanosleep(&delay, NULL);
    TEST_ASSERT_TRUE(send_text("\x03"));
    TEST_ASSERT_TRUE(send_text("printf '__%s__\\n' INTERRUPTED\n"));
    TEST_ASSERT_TRUE(await_text("__INTERRUPTED__"));
    TEST_ASSERT_TRUE(send_text("exit\n"));
    for (int i = 0; i < 200 && pe_terminal_process_running(process); i++) {
        struct timespec pause = {0, 5000000};
        nanosleep(&pause, NULL);
    }
    TEST_ASSERT_FALSE(pe_terminal_process_running(process));
}

static void test_invalid_root_reports_error(void) {
    char error[256] = {0};
    process = pe_terminal_process_start("/penguin/no-such-terminal-root", "/bin/sh", 4, 20, error,
                                        sizeof error);
    TEST_ASSERT_NULL(process);
    TEST_ASSERT_TRUE(*error != 0);
}
#else
static void test_unavailable_backend_reports_error(void) {
    char error[256] = {0};
    process = pe_terminal_process_start(".", NULL, 4, 20, error, sizeof error);
    TEST_ASSERT_NULL(process);
    TEST_ASSERT_TRUE(*error != 0);
}
#endif
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_colors_cursor_and_carriage_return);
    RUN_TEST(test_utf8_and_alternate_screen);
    RUN_TEST(test_scrollback_resize_and_copy);
    RUN_TEST(test_keyboard_and_partial_output);
    RUN_TEST(test_bracketed_paste_and_limit);
#ifndef _WIN32
    RUN_TEST(test_real_pty_commands_resize_interrupt_and_exit);
    RUN_TEST(test_invalid_root_reports_error);
#else
    RUN_TEST(test_unavailable_backend_reports_error);
#endif
    return UNITY_END();
}
