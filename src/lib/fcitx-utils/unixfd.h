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
#ifndef _FCITX_UTILS_UNIXFD_H_
#define _FCITX_UTILS_UNIXFD_H_

#include "fcitxutils_export.h"
#include <atomic>
#include <memory>

namespace fcitx {

class UnixFDPrivate;

class FCITXUTILS_EXPORT UnixFD {
public:
    UnixFD() noexcept;
    UnixFD(int fd);
    UnixFD(const UnixFD &other) = delete;
    UnixFD(UnixFD &&other) noexcept;
    ~UnixFD() noexcept;

    static UnixFD own(int fd) {
        UnixFD unixFD;
        unixFD.give(fd);
        return unixFD;
    }

    UnixFD &operator=(UnixFD &&other) noexcept;

    bool isValid() const noexcept;
    void set(int fd);
    void reset() noexcept;
    int release() noexcept;
    int fd() const noexcept;

    void give(int fd) noexcept;

private:
    int fd_;
};
}

#endif // _FCITX_UTILS_UNIXFD_H_
