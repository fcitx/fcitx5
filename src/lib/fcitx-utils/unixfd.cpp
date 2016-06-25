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
    UnixFDPrivate() : m_fd(-1) {}
    ~UnixFDPrivate() {
        if (m_fd != -1) {
            int ret;
            do {
                ret = close(m_fd);
            } while (ret == -1 && errno == EINTR);
        }
    }
    std::atomic_int m_fd;
};

UnixFD::UnixFD(int fd) { set(fd); }

UnixFD::UnixFD(UnixFD &&other) noexcept : d(std::move(other.d)) {}

UnixFD::~UnixFD() {}

UnixFD &UnixFD::operator=(UnixFD other) {
    using std::swap;
    swap(d, other.d);
    return *this;
}

bool UnixFD::isValid() const { return d && d->m_fd != -1; }

int UnixFD::fd() const { return d ? d->m_fd.load() : -1; }

void UnixFD::give(int fd) {
    if (fd == -1) {
        d.reset();
    } else {
        if (!d) {
            d = std::make_unique<UnixFDPrivate>();
        }

        d->m_fd = fd;
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

        d->m_fd = nfd;
    }
}

int UnixFD::release() {
    int fd = d->m_fd.exchange(-1);
    d.reset();
    return fd;
}
}
