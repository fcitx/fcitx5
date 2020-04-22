//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include "misc.h"
#include <sys/wait.h>
#include <unistd.h>
#include <fmt/format.h>
#include "log.h"

namespace fcitx {

void startProcess(const std::vector<std::string> &args,
                  const std::string &workingDirectory) {
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
}

std::string getProcessName(pid_t pid) {
#if defined(__linux__)
    auto path = fmt::format("/proc/{}/exe", pid);
    if (auto link = fs::readlink(path)) {
        return fs::baseName(*link);
    }
    return {};
#elif defined(LIBKVM_FOUND)
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
#else
    return result;
#endif
}

} // namespace fcitx
