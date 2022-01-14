/*
 * SPDX-FileCopyrightText: 2002-2005 Yuking <yuking_net@sohu.com>
 * SPDX-FileCopyrightText: 2010-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "config.h"

#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "fcitx-utils/fs.h"
#include "fcitx/instance.h"
#include "errorhandler.h"

#if defined(EXECINFO_FOUND)
#include <execinfo.h>
#endif

#ifndef SIGUNUSED
#define SIGUNUSED 32
#endif

#define MINIMAL_BUFFER_SIZE 256
#define BACKTRACE_SIZE 32

extern int selfpipe[2];
extern std::string crashlog;

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
        case SIGURG:
            signal(signo, SIG_IGN);
            break;
        default:
            signal(signo, OnException);
        }
    }

#if defined(EXECINFO_FOUND)
    // Call backtrace ahead so we do not need lock in backtrace.
    void *array[BACKTRACE_SIZE] = {
        nullptr,
    };
    (void)backtrace(array, BACKTRACE_SIZE);
#endif
}

static inline void BufferReset(MinimalBuffer *buffer) { buffer->offset = 0; }

static inline void BufferAppendUInt64(MinimalBuffer *buffer, uint64_t number,
                                      int radix) {
    int i = 0;
    while (buffer->offset + i < MINIMAL_BUFFER_SIZE) {
        const int tmp = number % radix;
        number /= radix;
        buffer->buffer[buffer->offset + i] =
            (tmp < 10 ? '0' + tmp : 'a' + tmp - 10);
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
    if (fd >= 0 && fd != STDERR_FILENO) {
        fcitx::fs::safeWrite(fd, str, len);
    }
    fcitx::fs::safeWrite(STDERR_FILENO, str, len);
}

static inline void _write_string(int fd, const char *str) {
    _write_string_len(fd, str, strlen(str));
}

static inline void _write_buffer(int fd, const MinimalBuffer *buffer) {
    _write_string_len(fd, buffer->buffer, buffer->offset);
}

void OnException(int signo) {
    if (signo == SIGCHLD) {
        return;
    }

    MinimalBuffer buffer;
    int fd = -1;

    if (!crashlog.empty() && (signo == SIGSEGV || signo == SIGABRT)) {
        fd = open(crashlog.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    }

    /* print signal info */
    BufferReset(&buffer);
    BufferAppendUInt64(&buffer, signo, 10);
    _write_string(fd, "=========================\n");
    _write_string(fd, "Fcitx " FCITX_VERSION_STRING " -- Get Signal No.: ");
    _write_buffer(fd, &buffer);
    _write_string(fd, "\n");

    /* print time info */
    time_t t = time(nullptr);
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
    void *array[BACKTRACE_SIZE] = {
        nullptr,
    };

    int size = backtrace(array, BACKTRACE_SIZE);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    if (fd >= 0) {
        backtrace_symbols_fd(array, size, fd);
    }
#endif

    if (fd >= 0) {
        close(fd);
    }

    switch (signo) {
    case SIGABRT:
    case SIGSEGV:
    case SIGBUS:
    case SIGILL:
    case SIGFPE:
        signal(signo, SIG_DFL);
        kill(getpid(), signo);
        break;
    default: {
        uint8_t sig = 0;
        if (signo < 0xff) {
            sig = (uint8_t)(signo & 0xff);
        }
        fcitx::fs::safeWrite(selfpipe[1], &sig, 1);
        signal(signo, OnException);
    } break;
    }
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
