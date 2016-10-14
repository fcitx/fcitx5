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

#include "unixfd.h"
#include <atomic>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

namespace fcitx {

class UnixFDPrivate {
public:
    UnixFDPrivate() : fd_(-1) {}
    ~UnixFDPrivate() {
        if (fd_ != -1) {
            int ret;
            do {
                ret = close(fd_);
            } while (ret == -1 && errno == EINTR);
        }
    }
    std::atomic_int fd_;
};

UnixFD::UnixFD(int fd) { set(fd); }

UnixFD::UnixFD(UnixFD &&other) noexcept : d(std::move(other.d)) {}

UnixFD::~UnixFD() {}

UnixFD &UnixFD::operator=(UnixFD other) {
    using std::swap;
    swap(d, other.d);
    return *this;
}

bool UnixFD::isValid() const { return d && d->fd_ != -1; }

int UnixFD::fd() const { return d ? d->fd_.load() : -1; }

void UnixFD::give(int fd) {
    if (fd == -1) {
        d.reset();
    } else {
        if (!d) {
            d = std::make_unique<UnixFDPrivate>();
        }

        d->fd_ = fd;
    }
}

void UnixFD::set(int fd) {
    if (fd == -1) {
        d.reset();
    } else {
        if (!d) {
            d = std::make_unique<UnixFDPrivate>();
        }

        int nfd = ::fcntl(fd, F_DUPFD_CLOEXEC, 0);
        if (nfd == -1) {
            // FIXME: throw exception
            return;
        }

        d->fd_ = nfd;
    }
}

int UnixFD::release() {
    int fd = d->fd_.exchange(-1);
    d.reset();
    return fd;
}
}
