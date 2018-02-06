//
// Copyright (C) 2016~2016 by CSSlayer
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
#include "fcitx-utils/log.h"
#include "fcitx-utils/unixfd.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace fcitx;

bool fd_is_valid(int fd) { return fcntl(fd, F_GETFD) != -1 || errno != EBADF; }

int main() {
    char fname[] = "XXXXXX";
    umask(S_IXUSR | S_IRWXG | S_IRWXO);
    int f = mkstemp(fname);
    FCITX_ASSERT(f != -1);
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
    {
        UnixFD fd1(f);
        FCITX_ASSERT(fd1.fd() != f);
        fdnum = fd1.release();
        FCITX_ASSERT(fd1.fd() == -1);
    }
    FCITX_ASSERT(fd_is_valid(fdnum));

    unlink(fname);
    return 0;
}
