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
#ifndef _FCITX_UTILS_DBUS_UTILS_P_H_
#define _FCITX_UTILS_DBUS_UTILS_P_H_

#include <string>
#include <vector>

namespace fcitx {
namespace dbus {

static inline std::string::const_iterator
findMatched(std::string::const_iterator start, std::string::const_iterator end,
            char symbolOpen, char symbolEnd) {
    int c = 1;
    while (start != end) {
        if (*start == symbolOpen) {
            c += 1;
        } else if (*start == symbolEnd) {
            c -= 1;
        }
        ++start;
        if (c == 0) {
            break;
        }
    }
    return start;
}

static inline std::string::const_iterator
consumeSingle(std::string::const_iterator start,
              std::string::const_iterator end) {
    if (*start == '(') {
        return findMatched(start + 1, end, '(', ')');
    } else if (*start == '{') {
        return findMatched(start + 1, end, '{', '}');
    } else if (*start == 'a') {
        return consumeSingle(start + 1, end);
    }
    return start + 1;
}

static inline std::vector<std::string>
splitDBusSignature(const std::string &s) {
    std::vector<std::string> result;
    auto iter = s.begin();
    while (iter != s.end()) {
        auto next = consumeSingle(iter, s.end());
        result.emplace_back(iter, next);
        iter = next;
    }
    return result;
}

} // namespace dbus
} // namespace fcitx

#endif // _FCITX_UTILS_DBUS_UTILS_P_H_
