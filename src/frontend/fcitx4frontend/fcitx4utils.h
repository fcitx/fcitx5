/*
 * SPDX-FileCopyrightText: 2020-2021 Vifly <viflythink@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _FCITX_FCITX4UTILS_H_
#define _FCITX_FCITX4UTILS_H_

#include <libgen.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dbus_public.h>
#include <expat_config.h>

#define allocCatCStr(dest, strs...)                                            \
    do {                                                                       \
        const char *__str_list[] = {strs};                                     \
        size_t __cat_str_n = sizeof(__str_list) / sizeof(char *);              \
        size_t __size_list[sizeof(__str_list) / sizeof(char *)];               \
        size_t __total_size = cStrLens(__cat_str_n, __str_list, __size_list);  \
        dest = static_cast<char *>(malloc(__total_size));                      \
        catCStr(dest, __cat_str_n, __str_list, __size_list);                   \
    } while (0)

size_t cStrLens(size_t n, const char **str_list, size_t *size_list);
void catCStr(char *out, size_t n, const char **str_list,
             const size_t *size_list);
char **FcitxXDGGetPath(size_t *len, const char *homeEnv,
                       const char *homeDefault, const char *suffixHome,
                       const char *dirsDefault, const char *suffixGlobal);
FILE *Fcitx4XDGGetFile(const char *fileName, char **path, const char *mode,
                       size_t len, char **retFile);
char **Fcitx4XDGGetPathUserWithPrefix(size_t *len, const char *prefix);
FILE *Fcitx4XDGGetFileUserWithPrefix(const char *prefix, const char *fileName,
                                     const char *mode, char **retFile);

static inline void combinePathWithLen(char *dest, const char *str1, size_t len1,
                                      const char *str2, size_t len2) {
    const char *str_list[] = {str1, "/", str2};
    size_t size_list[] = {len1, 1, len2};
    catCStr(dest, 3, str_list, size_list);
}

#endif // _FCITX_FCITX4UTILS_H_
