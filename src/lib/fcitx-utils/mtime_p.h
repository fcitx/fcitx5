/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_MTIME_P_H_
#define _FCITX_UTILS_MTIME_P_H_

#include <sys/stat.h>
#include <cstdint>
#include <type_traits>

namespace fcitx {

struct Timespec {
    int64_t sec;
    int64_t nsec;
};

template <typename T>
inline std::enable_if_t<(&T::st_mtim, true), Timespec>
modifiedTime(const T &p) {
    return {p.st_mtim.tv_sec, p.st_mtim.tv_nsec};
}

// This check is necessary because on FreeBSD st_mtimespec is defined as
// st_mtim. This would cause a redefinition.
#if !defined(st_mtimespec)
template <typename T>
inline std::enable_if_t<(&T::st_mtimespec, true), Timespec>
modifiedTime(const T &p) {
    return {p.st_mtimespec.tv_sec, p.st_mtimespec.tv_nsec};
}
#endif

#if !defined(st_mtimensec) && !defined(__alpha__)
template <typename T>
inline std::enable_if_t<(&T::st_mtimensec, true), Timespec>
modifiedTime(const T &p) {
    return {p.st_mtime, p.st_mtimensec};
}
#endif

} // namespace fcitx

#endif // _FCITX_UTILS_MTIME_P_H_
