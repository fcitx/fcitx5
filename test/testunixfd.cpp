/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fcitx-utils/log.h"
#include "fcitx-utils/unixfd.h"

using namespace fcitx;

bool fd_is_valid(int fd) { return fcntl(fd, F_GETFD) != -1 || errno != EBADF; }

int main() {
    char fname[] = "XXXXXX";
    umask(S_IXUSR | S_IRWXG | S_IRWXO);
    int f = mkstemp(fname);
    FCITX_ASSERT(f != -1);
    // Test empty unixfd.
    {
        UnixFD fd;
        FCITX_ASSERT(fd.fd() == -1);
    }

    int fdnum = -1;
    {
        UnixFD fd;
        fd.set(f);
        FCITX_ASSERT(fd.fd() != f);
        FCITX_ASSERT(fd.fd() != -1);
        fdnum = fd.fd();
    }

    // Test release and close.
    FCITX_ASSERT(!fd_is_valid(fdnum));
    {
        UnixFD fd(f);
        FCITX_ASSERT(fd.fd() != f);
        FCITX_ASSERT(fd.fd() != -1);
        fdnum = fd.release();
        FCITX_ASSERT(fd.fd() == -1);
    }
    FCITX_ASSERT(fd_is_valid(fdnum));
    close(fdnum);
    FCITX_ASSERT(!fd_is_valid(fdnum));
    // Test release.
    {
        UnixFD fd1(f);
        FCITX_ASSERT(fd1.fd() != f);
        fdnum = fd1.release();
        FCITX_ASSERT(fd1.fd() == -1);
    }
    FCITX_ASSERT(fd_is_valid(fdnum));
    {
        UnixFD fd1 = UnixFD::own(fdnum);
        FCITX_ASSERT(fd1.fd() == fdnum);
    }
    FCITX_ASSERT(!fd_is_valid(fdnum));
    // Test set to invalid fd.
    {
        UnixFD fd1(f);
        FCITX_ASSERT(fd1.fd() != f);
        fdnum = fd1.fd();
        fd1.set(-1);
        FCITX_ASSERT(fd1.fd() == -1);
    }
    FCITX_ASSERT(!fd_is_valid(fdnum));

    unlink(fname);
    return 0;
}
