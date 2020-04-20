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

#include "unixfd.h"
#include <errno.h>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>

namespace fcitx {

UnixFD::UnixFD() noexcept = default;
UnixFD::UnixFD(int fd) { set(fd); }

UnixFD::UnixFD(UnixFD &&other) noexcept {
    operator=(std::forward<UnixFD>(other));
}

UnixFD::~UnixFD() noexcept { reset(); }

UnixFD &UnixFD::operator=(UnixFD &&other) noexcept {
    // Close current, move over the other one.
    using std::swap;
    reset();
    swap(fd_, other.fd_);
    return *this;
}

bool UnixFD::isValid() const noexcept { return fd_ != -1; }

int UnixFD::fd() const noexcept { return fd_; }

void UnixFD::give(int fd) noexcept {
    if (fd == -1) {
        reset();
    } else {
        fd_ = fd;
    }
}

void UnixFD::set(int fd) {
    if (fd == -1) {
        reset();
    } else {
        int nfd = ::fcntl(fd, F_DUPFD_CLOEXEC, 0);
        if (nfd == -1) {
            throw std::runtime_error("Failed to dup file descriptor");
        }

        fd_ = nfd;
    }
}

void UnixFD::reset() noexcept {
    if (fd_ != -1) {
        int ret;
        do {
            ret = close(fd_);
        } while (ret == -1 && errno == EINTR);
        fd_ = -1;
    }
}

int UnixFD::release() noexcept {
    int fd = fd_;
    fd_ = -1;
    return fd;
}
} // namespace fcitx
