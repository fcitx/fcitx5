/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "plasmathemewatchdog.h"
#include <sys/wait.h>
#include <unistd.h>
#include <stdexcept>
#include "fcitx-utils/event.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/standardpath.h"
#include "common.h"

#if defined(__FreeBSD__)
#include <sys/procctl.h>
#elif defined(__linux__)
#include <sys/prctl.h>
#endif

namespace fcitx::classicui {

#define PLASMA_THEME_GENERATOR "fcitx5-plasma-theme-generator";

bool PlasmaThemeWatchdog::isAvailable() {
    static const std::string binaryName = PLASMA_THEME_GENERATOR;
    return StandardPath::hasExecutable(binaryName);
}

PlasmaThemeWatchdog::PlasmaThemeWatchdog(EventLoop *event,
                                         std::function<void()> callback)
    : callback_(std::move(callback)) {
    int pipefd[2];
    int ret = ::pipe(pipefd);
    if (ret == -1) {
        throw std::runtime_error("Failed to create pipe");
    }
    ::fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
    ::fcntl(pipefd[0], F_SETFL, ::fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);
    ::fcntl(pipefd[1], F_SETFL, ::fcntl(pipefd[1], F_GETFL) | O_NONBLOCK);
    auto pid = fork();
    if (pid == 0) {
        close(pipefd[0]);
#if defined(__FreeBSD__)
        int procctl_value = SIGKILL;
        procctl(P_PID, 0, PROC_PDEATHSIG_CTL, &procctl_value);
#elif defined(__linux__)
        prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
        char arg0[] = PLASMA_THEME_GENERATOR;
        char arg1[] = "--fd";
        std::string value = std::to_string(pipefd[1]);
        char *args[] = {arg0, arg1, value.data(), nullptr};
        execvp(arg0, args);
        _exit(1);
    } else {
        close(pipefd[1]);
        monitorFD_.give(pipefd[0]);
        generator_ = pid;
        running_ = true;
        ioEvent_ = event->addIOEvent(
            monitorFD_.fd(),
            {IOEventFlag::In, IOEventFlag::Err, IOEventFlag::Hup},
            [this, event](EventSourceIO *, int fd, IOEventFlags flags) {
                if ((flags & IOEventFlag::Err) || (flags & IOEventFlag::Hup)) {
                    cleanup();
                    return true;
                }
                if (flags & IOEventFlag::In) {
                    uint8_t dummy;
                    while (fs::safeRead(fd, &dummy, sizeof(dummy)) > 0) {
                    }
                    timerEvent_ = event->addTimeEvent(
                        CLOCK_MONOTONIC, now(CLOCK_MONOTONIC) + 1000000, 0,
                        [this](EventSourceTime *, uint64_t) {
                            callback_();
                            return true;
                        });
                }
                return true;
            });
    }
}
PlasmaThemeWatchdog::~PlasmaThemeWatchdog() {
    destruct_ = true;
    cleanup();
}

void PlasmaThemeWatchdog::cleanup() {
    running_ = false;
    CLASSICUI_INFO() << "Cleanup Plasma Theme generator.";
    if (!destruct_) {
        // For certain distribution, it is possible that
        // fcitx5-plasma-theme-generator command presents, but not usable since
        // plasma is set to be an optional dependency to fcitx5-configtool. If
        // that happens, we need to notify to reload the theme.
        callback_();
    }
    if (generator_ == 0) {
        return;
    }
    int stat = 0;
    int ret;
    kill(generator_, SIGKILL);
    do {
        ret = waitpid(generator_, &stat, WNOHANG);
    } while (ret == -1 && errno == EINTR);
    generator_ = 0;
    ioEvent_.reset();
}

} // namespace fcitx::classicui