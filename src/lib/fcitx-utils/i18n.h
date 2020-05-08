/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
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
} // namespace fcitx

#ifndef FCITX_NO_I18N_MACRO

#ifdef FCITX_GETTEXT_DOMAIN
#define _(x) ::fcitx::translateDomain(FCITX_GETTEXT_DOMAIN, x)
#define C_(c, x) ::fcitx::translateDomainCtx(FCITX_GETTEXT_DOMAIN, c, x)
#else
#define _(x) ::fcitx::translate(x)
#define C_(c, x) ::fcitx::translateCtx(c, x)
#endif

#define D_(d, x) ::fcitx::translateDomain(d, x)

#define NC_(c, x) (x)
#define N_(x) (x)

#endif

#endif // _FCITX_UTILS_I18N_H_
