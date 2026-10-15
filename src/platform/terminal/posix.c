#define _XOPEN_SOURCE 700
#include "platform/terminal/process.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char **environ;

struct PeTerminalProcess {
    int master;
    pid_t pid;
    bool reaped;
};

static void child_fail(int fd) {
    int value = errno;
    ssize_t ignored = write(fd, &value, sizeof value);
    (void)ignored;
    _exit(127);
}

PeTerminalProcess *pe_terminal_process_start(const char *root, const char *shell, int rows,
                                             int cols, char *error, size_t size) {
    if (!root || rows < 1 || cols < 1 || rows > 100 || cols > 240) {
        snprintf(error, size, "Invalid terminal directory or dimensions");
        return NULL;
    }
    if (!shell) {
        shell = getenv("SHELL");
        if (!shell || shell[0] != '/' || access(shell, X_OK)) {
            shell = "/bin/sh";
        }
    }
    if (shell[0] != '/' || access(shell, X_OK)) {
        snprintf(error, size, "Terminal shell is not executable");
        return NULL;
    }
    PeTerminalProcess *p = calloc(1, sizeof *p);
    if (!p) {
        snprintf(error, size, "Out of memory starting terminal");
        return NULL;
    }
    p->master = -1;
    int slave = -1, channel[2] = {-1, -1};
    char **environment = NULL;
    p->master = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (p->master < 0 || grantpt(p->master) || unlockpt(p->master)) {
        goto fail;
    }
    const char *name = ptsname(p->master);
    if (!name || (slave = open(name, O_RDWR | O_NOCTTY | O_CLOEXEC)) < 0) {
        goto fail;
    }
    struct winsize dimensions = {(unsigned short)rows, (unsigned short)cols, 0, 0};
    if (ioctl(slave, TIOCSWINSZ, &dimensions) || pipe(channel)) {
        goto fail;
    }
    if (fcntl(channel[0], F_SETFD, FD_CLOEXEC) || fcntl(channel[1], F_SETFD, FD_CLOEXEC)) {
        goto fail;
    }
    size_t count = 0;
    while (environ[count]) {
        count++;
    }
    environment = calloc(count + 3, sizeof *environment);
    if (!environment) {
        goto fail;
    }
    size_t n = 0;
    for (size_t i = 0; i < count; i++) {
        if (strncmp(environ[i], "TERM=", 5) && strncmp(environ[i], "COLORTERM=", 10) &&
            strncmp(environ[i], "LINES=", 6) && strncmp(environ[i], "COLUMNS=", 8)) {
            environment[n++] = environ[i];
        }
    }
    environment[n++] = "TERM=xterm-256color";
    environment[n++] = "COLORTERM=truecolor";
    char *argv[] = {(char *)shell, "-i", NULL};
    p->pid = fork();
    if (p->pid < 0) {
        goto fail;
    }
    if (p->pid == 0) {
        /* No allocation/SDL/stdio in the child between fork and exec. */
        close(channel[0]);
        close(p->master);
        if (setsid() < 0 || ioctl(slave, TIOCSCTTY, 0) < 0 || chdir(root)) {
            child_fail(channel[1]);
        }
        for (int fd = 0; fd < 3; fd++) {
            if (dup2(slave, fd) < 0 || fcntl(fd, F_SETFD, 0) < 0) {
                child_fail(channel[1]);
            }
        }
        if (slave > 2) {
            close(slave);
        }
        execve(shell, argv, environment);
        child_fail(channel[1]);
    }
    close(slave);
    slave = -1;
    close(channel[1]);
    channel[1] = -1;
    free(environment);
    environment = NULL;
    struct pollfd descriptor = {channel[0], POLLIN | POLLHUP, 0};
    int ready;
    do {
        ready = poll(&descriptor, 1, 1000);
    } while (ready < 0 && errno == EINTR);
    int child_error = 0;
    ssize_t received = ready > 0 ? read(channel[0], &child_error, sizeof child_error) : -1;
    if (received != 0) {
        errno = child_error ? child_error : EIO;
        goto fail;
    }
    close(channel[0]);
    channel[0] = -1;
    if (fcntl(p->master, F_SETFL, O_NONBLOCK)) {
        goto fail;
    }
    return p;
fail:
    {
        int saved = errno;
        if (slave >= 0) {
            close(slave);
        }
        if (channel[0] >= 0) {
            close(channel[0]);
        }
        if (channel[1] >= 0) {
            close(channel[1]);
        }
        free(environment);
        pe_terminal_process_free(p);
        snprintf(error, size, "Cannot start terminal: %s", strerror(saved));
        return NULL;
    }
}

int pe_terminal_process_read(PeTerminalProcess *p, char *buffer, size_t capacity) {
    ssize_t n;
    do {
        n = read(p->master, buffer, capacity);
    } while (n < 0 && errno == EINTR);
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return 0;
    }
    return n > 0 ? (int)n : -1;
}

int pe_terminal_process_write(PeTerminalProcess *p, const char *bytes, size_t length) {
    ssize_t n;
    do {
        n = write(p->master, bytes, length);
    } while (n < 0 && errno == EINTR);
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return 0;
    }
    return (int)n;
}

bool pe_terminal_process_resize(PeTerminalProcess *p, int rows, int cols) {
    if (rows < 1 || cols < 1 || rows > 100 || cols > 240) {
        return false;
    }
    struct winsize size = {(unsigned short)rows, (unsigned short)cols, 0, 0};
    return ioctl(p->master, TIOCSWINSZ, &size) == 0;
}

bool pe_terminal_process_running(PeTerminalProcess *p) {
    if (!p || p->reaped) {
        return false;
    }
    int status;
    pid_t result = waitpid(p->pid, &status, WNOHANG);
    if (result == p->pid || (result < 0 && errno == ECHILD)) {
        p->reaped = true;
    }
    return !p->reaped;
}

void pe_terminal_process_free(PeTerminalProcess *p) {
    if (!p) {
        return;
    }
    pid_t foreground = p->master >= 0 ? tcgetpgrp(p->master) : -1;
    bool running = p->pid > 0 && pe_terminal_process_running(p);
    if (running) {
        kill(-p->pid, SIGHUP);
        kill(p->pid, SIGHUP);
    }
    bool child_group = foreground > 1 && foreground != getpgrp() && foreground != p->pid;
    if (child_group) {
        kill(-foreground, SIGHUP);
    }
    if (running) {
        for (int i = 0; i < 10 && pe_terminal_process_running(p); i++) {
            struct timespec delay = {0, 10000000};
            nanosleep(&delay, NULL);
        }
    }
    if (child_group) {
        kill(-foreground, SIGKILL);
    }
    if (p->pid > 0 && !p->reaped && pe_terminal_process_running(p)) {
        kill(-p->pid, SIGKILL);
        kill(p->pid, SIGKILL);
        while (waitpid(p->pid, NULL, 0) < 0 && errno == EINTR) {
        }
    }
    if (p->master >= 0) {
        close(p->master);
    }
    free(p);
}
