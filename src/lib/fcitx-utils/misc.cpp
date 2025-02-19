/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "misc.h"
#include <unistd.h>
#include <cstdio>
#include <string>
#include <vector>
#include <format>
#include "fs.h"
#include "log.h"
#include "misc_p.h"

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#if defined(LIBKVM_FOUND)
#include <fcntl.h>
#include <kvm.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#if defined(__FreeBSD__)
#include <sys/user.h>
#endif
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#include <libproc.h>
#endif
#endif

namespace fcitx {

void startProcess(const std::vector<std::string> &args,
                  const std::string &workingDirectory) {
#if defined(_WIN32) || (defined(__APPLE__) && TARGET_OS_IPHONE)
    FCITX_UNUSED(args);
    FCITX_UNUSED(workingDirectory);
    FCITX_ERROR() << "Not implemented";
    return;
#else
    /* exec command */
    pid_t child_pid;

    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
    } else if (child_pid == 0) { /* child process  */
        setsid();
        pid_t grandchild_pid;

        grandchild_pid = fork();
        if (grandchild_pid < 0) {
            perror("fork");
            _exit(1);
        } else if (grandchild_pid == 0) { /* grandchild process  */
            if (!workingDirectory.empty()) {
                if (chdir(workingDirectory.data()) != 0) {
                    FCITX_WARN() << "Failed to change working directory";
                }
            }
            std::vector<char *> argv;
            argv.reserve(args.size() + 1);
            // const_cast is needed because execvp prototype wants an
            // array of char*, not const char*.
            for (auto const &a : args) {
                argv.emplace_back(const_cast<char *>(a.c_str()));
            }
            // nullptr terminate
            argv.push_back(nullptr);
            execvp(argv[0], argv.data());
            perror("execvp");
            _exit(1);
        } else {
            _exit(0);
        }
    } else { /* parent process */
        int status;
        waitpid(child_pid, &status, 0);
    }
#endif
}

std::string getProcessName(pid_t pid) {
#if defined(LIBKVM_FOUND)
#if defined(__NetBSD__) || defined(__OpenBSD__)
    kvm_t *vm = kvm_open(nullptr, nullptr, nullptr, KVM_NO_FILES, nullptr);
#else
    kvm_t *vm = kvm_open(0, "/dev/null", 0, O_RDONLY, nullptr);
#endif
    if (vm == 0) {
        return {};
    }

    std::string result;
    do {
        int cnt;
#ifdef __NetBSD__
        struct kinfo_proc2 *kp = kvm_getproc2(vm, KERN_PROC_PID, pid,
                                              sizeof(struct kinfo_proc2), &cnt);
#else
        struct kinfo_proc *kp = kvm_getprocs(vm, KERN_PROC_PID, pid, &cnt);
#endif
        if ((cnt != 1) || (kp == 0)) {
            break;
        }
        int i;
        for (i = 0; i < cnt; i++)
#if defined(__NetBSD__) || defined(__OpenBSD__)
            if (kp->p_pid == pid)
#else
            if (kp->ki_pid == pid)
#endif
                break;
        if (i != cnt) {
#if defined(__NetBSD__) || defined(__OpenBSD__)
            result = kp->p_comm;
#else
            result = kp->ki_comm;
#endif
        }
    } while (0);
    kvm_close(vm);
    return result;
#elif defined(__APPLE__)
#if TARGET_OS_IPHONE
    FCITX_UNUSED(pid);
    FCITX_ERROR() << "Not implemented";
    return "";
#else
    std::string result;
    result.reserve(2 * MAXCOMLEN);

    if (proc_name(pid, result.data(), 2 * MAXCOMLEN)) {
        return {};
    }
    return result;
#endif
#elif defined(_WIN32)
    FCITX_UNUSED(pid);
    return {};
#else
    auto path = std::format("/proc/{}/exe", pid);
    if (auto link = fs::readlink(path)) {
        return fs::baseName(*link);
    }
    return {};
#endif
}

#ifndef _WIN32

ssize_t getline(UniqueCPtr<char> &lineptr, size_t *n, std::FILE *stream) {
    auto *lineRawPtr = lineptr.release();
    auto ret = getline(&lineRawPtr, n, stream);
    lineptr.reset(lineRawPtr);
    return ret;
}

#endif

bool isInFlatpak() {
#ifdef __APPLE__
    return false;
#else
    static const bool flatpak = []() {
        if (checkBoolEnvVar("FCITX_OVERRIDE_FLATPAK")) {
            return true;
        }
        return fs::isreg("/.flatpak-info");
    }();
    return flatpak;
#endif
}

} // namespace fcitx
