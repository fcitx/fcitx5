/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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

std::string translate(const std::string &s);
const char *translate(const char *s);
std::string translateCtx(const char *ctx, const std::string &s);
const char *translateCtx(const char *ctx, const char *s);

std::string translateDomain(const char *domain, const std::string &s);
const char *translateDomain(const char *domain, const char *s);
std::string translateDomainCtx(const char *domain, const char *ctx,
                               const std::string &s);

const char *translateDomainCtx(const char *domain, const char *ctx,
                               const char *s);
void registerDomain(const char *domain, const char *dir);
}

#ifndef FCITX_NO_I18N_MACRO

#ifdef FCITX_GETTEXT_DOMAIN
#define _(x) ::fcitx::translateDomain(FCITX_GETTEXT_DOMAIN, x)
#define C_(c, x) ::fcitx::translateDomainCtx(FCITX_GETTEXT_DOMAIN, c, x)
#else
#define _(x) ::fcitx::translate(x)
#define C_(c, x) ::fcitx::translateCtx(FCITX_GETTEXT_DOMAIN, c, x)
#endif

#define D_(d, x) ::fcitx::translateDomain(d, x)

#define NC_(c, x) (x)
#define N_(x) (x)

#endif

#endif // _FCITX_UTILS_I18N_H_
