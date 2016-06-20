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

#include <memory>
#include "fcitxutils_export.h"

namespace fcitx {

class UnixFDPrivate;

class FCITXUTILS_EXPORT UnixFD {
public:
    UnixFD(int fd = -1);
    UnixFD(const UnixFD &other) = delete;
    UnixFD(UnixFD &&other) noexcept;
    ~UnixFD();

    static UnixFD own(int fd) {
        UnixFD unixFD;
        unixFD.give(fd);
        return unixFD;
    }

    UnixFD &operator=(UnixFD other);

    bool isValid() const;
    void set(int fd);
    int release();
    int fd() const;

    void give(int fd);

private:
    std::unique_ptr<UnixFDPrivate> d;
};
}

#endif // _FCITX_UTILS_UNIXFD_H_
