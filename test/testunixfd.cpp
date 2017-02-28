/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#include "fcitx-utils/unixfd.h"
#include <cassert>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace fcitx;

bool fd_is_valid(int fd) { return fcntl(fd, F_GETFD) != -1 || errno != EBADF; }

int main() {
    char fname[] = "XXXXXX";
    umask(S_IXUSR | S_IRWXG | S_IRWXO);
    int f = mkstemp(fname);
    assert(f != -1);
    {
        UnixFD fd;
        assert(fd.fd() == -1);
    }

    int fdnum = -1;
    {
        UnixFD fd;
        fd.set(f);
        assert(fd.fd() != f);
        assert(fd.fd() != -1);
        fdnum = fd.fd();
    }

    assert(!fd_is_valid(fdnum));
    {
        UnixFD fd(f);
        assert(fd.fd() != f);
        assert(fd.fd() != -1);
        fdnum = fd.release();
        assert(fd.fd() == -1);
    }
    assert(fd_is_valid(fdnum));
    close(fdnum);
    assert(!fd_is_valid(fdnum));
    {
        UnixFD fd1(f);
        assert(fd1.fd() != f);
        fdnum = fd1.release();
        assert(fd1.fd() == -1);
    }
    assert(fd_is_valid(fdnum));

    unlink(fname);
    return 0;
}
