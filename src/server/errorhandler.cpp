/*
 * Copyright (C) 2002~2005 by Yuking
 * yuking_net@sohu.com
 * Copyright (C) 2010~2015 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "errorhandler.h"
#include "fcitx-utils/fs.h"
#include "fcitx/instance.h"

#if defined(EXECINFO_FOUND)
#include <execinfo.h>
#endif

#ifndef SIGUNUSED
#define SIGUNUSED 32
#endif

#define MINIMAL_BUFFER_SIZE 256

extern int selfpipe[2];
extern char *crashlog;

typedef struct _MinimalBuffer {
    char buffer[MINIMAL_BUFFER_SIZE];
    int offset;
} MinimalBuffer;

void SetMyExceptionHandler(void) {
    int signo;

    for (signo = SIGHUP; signo < SIGUNUSED; signo++) {
        switch (signo) {
        case SIGTSTP:
        case SIGCONT:
            continue;
        case SIGALRM:
        case SIGPIPE:
        case SIGUSR2:
        case SIGWINCH:
            signal(signo, SIG_IGN);
            break;
        default:
            signal(signo, OnException);
        }
    }
}

static inline void BufferReset(MinimalBuffer *buffer) { buffer->offset = 0; }

static inline void BufferAppendUInt64(MinimalBuffer *buffer, uint64_t number, int radix) {
    int i = 0;
    while (buffer->offset + i < MINIMAL_BUFFER_SIZE) {
        const int tmp = number % radix;
        number /= radix;
        buffer->buffer[buffer->offset + i] = (tmp < 10 ? '0' + tmp : 'a' + tmp - 10);
        ++i;
        if (number == 0) {
            break;
        }
    }

    if (i > 1) {
        // reverse
        int j = 0;
        char *cursor = buffer->buffer + buffer->offset;
        for (j = 0; j < i / 2; j++) {
            char temp = cursor[j];
            cursor[j] = cursor[i - j - 1];
            cursor[i - j - 1] = temp;
        }
    }
    buffer->offset += i;
}

static inline void _write_string_len(int fd, const char *str, size_t len) {
    if (fd >= 0 && fd != STDERR_FILENO)
        fcitx::fs::safeWrite(fd, str, len);
    fcitx::fs::safeWrite(STDERR_FILENO, str, len);
}

static inline void _write_string(int fd, const char *str) { _write_string_len(fd, str, strlen(str)); }

static inline void _write_buffer(int fd, const MinimalBuffer *buffer) {
    _write_string_len(fd, buffer->buffer, buffer->offset);
}

void OnException(int signo) {
    if (signo == SIGCHLD)
        return;

    MinimalBuffer buffer;
    int fd = -1;

    if (crashlog && (signo == SIGSEGV || signo == SIGABRT))
        fd = open(crashlog, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    /* print signal info */
    BufferReset(&buffer);
    BufferAppendUInt64(&buffer, signo, 10);
    _write_string(fd, "=========================\n");
    _write_string(fd, "Fcitx " FCITX_VERSION_STRING " -- Get Signal No.: ");
    _write_buffer(fd, &buffer);
    _write_string(fd, "\n");

    /* print time info */
    time_t t = time(NULL);
    BufferReset(&buffer);
    BufferAppendUInt64(&buffer, t, 10);
    _write_string(fd, "Date: try \"date -d @");
    _write_buffer(fd, &buffer);
    _write_string(fd, "\" if you are using GNU date ***\n");

    /* print process info */
    BufferReset(&buffer);
    BufferAppendUInt64(&buffer, getpid(), 10);
    _write_string(fd, "ProcessID: ");
    _write_buffer(fd, &buffer);
    _write_string(fd, "\n");

#if defined(EXECINFO_FOUND)
#define BACKTRACE_SIZE 32
    void *array[BACKTRACE_SIZE] = {
        NULL,
    };

    int size = backtrace(array, BACKTRACE_SIZE);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    if (fd >= 0)
        backtrace_symbols_fd(array, size, fd);
#endif

    if (fd >= 0)
        close(fd);

    switch (signo) {
    case SIGABRT:
    case SIGSEGV:
    case SIGBUS:
    case SIGILL:
    case SIGFPE:
        _exit(1);
        break;
    default: {
        uint8_t sig = 0;
        if (signo < 0xff)
            sig = (uint8_t)(signo & 0xff);
        write(selfpipe[1], &sig, 1);
        signal(signo, OnException);
    } break;
    }
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
