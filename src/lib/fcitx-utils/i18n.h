/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_I18N_H_
#define _FCITX_UTILS_I18N_H_

#include <string>
#include <utility>
#include <fcitx-utils/fcitxutils_export.h>
#if __cplusplus >= 202002L
#include <format>
#endif

namespace fcitx {

FCITXUTILS_EXPORT std::string translate(const std::string &s);
FCITXUTILS_EXPORT const char *translate(const char *s);
FCITXUTILS_EXPORT std::string translateCtx(const char *ctx,
                                           const std::string &s);
FCITXUTILS_EXPORT const char *translateCtx(const char *ctx, const char *s);

FCITXUTILS_EXPORT std::string translateDomain(const char *domain,
                                              const std::string &s);
FCITXUTILS_EXPORT const char *translateDomain(const char *domain,
                                              const char *s);
FCITXUTILS_EXPORT std::string
translateDomainCtx(const char *domain, const char *ctx, const std::string &s);

FCITXUTILS_EXPORT const char *
translateDomainCtx(const char *domain, const char *ctx, const char *s);
FCITXUTILS_EXPORT void registerDomain(const char *domain, const char *dir);

#if __cplusplus >= 202002L
template <typename... Args>
auto translate(std::format_string<Args...> s, Args &&...args)
    -> std::enable_if_t<(sizeof...(Args) >= 1), std::string> {
    return std::vformat(translate(std::string(s.get())),
                        std::make_format_args(args...));
}

template <typename... Args>
auto translateCtx(const char *ctx, std::format_string<Args...> s,
                  Args &&...args)
    -> std::enable_if_t<(sizeof...(Args) >= 1), std::string> {
    return std::vformat(translateCtx(ctx, std::string(s.get())),
                        std::make_format_args(args...));
}

template <typename... Args>
auto translateDomain(const char *domain, std::format_string<Args...> s,
                     Args &&...args)
    -> std::enable_if_t<(sizeof...(Args) >= 1), std::string> {
    return std::vformat(translateDomain(domain, std::string(s.get())),
                        std::make_format_args(args...));
}

template <typename... Args>
auto translateDomainCtx(const char *domain, const char *ctx,
                        std::format_string<Args...> s, Args &&...args)
    -> std::enable_if_t<(sizeof...(Args) >= 1), std::string> {
    return std::vformat(translateDomainCtx(domain, ctx, std::string(s.get())),
                        std::make_format_args(args...));
}
#endif

} // namespace fcitx

#ifndef FCITX_NO_I18N_MACRO

#ifdef FCITX_GETTEXT_DOMAIN
#define _(...) ::fcitx::translateDomain(FCITX_GETTEXT_DOMAIN, __VA_ARGS__)
#define C_(c, ...)                                                             \
    ::fcitx::translateDomainCtx(FCITX_GETTEXT_DOMAIN, c, __VA_ARGS__)
#else
#define _(...) ::fcitx::translate(__VA_ARGS__)
#define C_(c, ...) ::fcitx::translateCtx(c, __VA_ARGS__)
#endif

#define D_(d, ...) ::fcitx::translateDomain(d, __VA_ARGS__)

#define NC_(c, x) (x)
#define N_(x) (x)

#endif

#endif // _FCITX_UTILS_I18N_H_
