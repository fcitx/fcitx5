/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_UTILS_I18N_H_
#define _FCITX_UTILS_I18N_H_

#include <string>

namespace fcitx {

const char *translate(const std::string &s);
const char *translate(const char *s);
const char *translateDomain(const char *domain, const std::string &s);
const char *translateDomain(const char *domain, const char *s);
}

#ifdef FCITX_GETTEXT_DOMAIN
#define _(x) translateDomain(FCITX_GETTEXT_DOMAIN, x)
#else
#define _(x) translate(x)
#endif

#define D_(d, x) translateDomain(d, x)

#endif // _FCITX_UTILS_I18N_H_
