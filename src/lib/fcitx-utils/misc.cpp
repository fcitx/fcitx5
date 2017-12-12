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
#include "log.h"
#include <sys/wait.h>
#include <unistd.h>

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
            // NULL terminate
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

} // namespace fcitx
