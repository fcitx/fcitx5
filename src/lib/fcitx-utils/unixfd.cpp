/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "unixfd.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

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
