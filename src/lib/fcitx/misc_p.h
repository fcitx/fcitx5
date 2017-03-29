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
#ifndef _FCITX_MISC_P_H_
#define _FCITX_MISC_P_H_

#include <libintl.h>
#include <string>
#include <type_traits>

namespace fcitx {

inline const char *_gettext(const std::string &s) {
    return ::gettext(s.c_str());
}

inline const char *_gettext(const char *s) { return ::gettext(s); }

inline const char *_dgettext(const char *domain, const std::string &s) {
    return ::dgettext(domain, s.c_str());
}

inline const char *_dgettext(const char *domain, const char *s) {
    return ::dgettext(domain, s);
}

#define _(X) fcitx::_gettext(X)
#define D_(D, X) fcitx::_dgettext(D, X)

template <typename M, typename K>
decltype(&std::declval<M>().begin()->second) findValue(M &&m, K &&key) {
    auto iter = m.find(key);
    if (iter != m.end()) {
        return &iter->second;
    }
    return nullptr;
}
}

#endif // _FCITX_MISC_P_H_
