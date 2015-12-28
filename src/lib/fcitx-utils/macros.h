/*
 * Copyright (C) 2015~2015 by CSSlayer
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
#ifndef _FCITX_UTILS_MACROS_H_
#define _FCITX_UTILS_MACROS_H_

// steal some Qt macro here

#define FCITX_DECLARE_PUBLIC(Class)                                            \
    inline Class *q_func() { return static_cast<Class *>(q_ptr); }             \
    inline const Class *q_func() const {                                       \
        return static_cast<const Class *>(q_ptr);                              \
    }                                                                          \
    friend class Class;

#define FCITX_DECLARE_PRIVATE(Class)                                           \
    inline Class##Private *d_func() { return d_ptr.get(); }                    \
    inline const Class##Private *d_func() const { return d_ptr.get(); }        \
    friend class Class##Private;

#define FCITX_D() auto *const d = d_func()
#define FCITX_Q() auto *const q = q_func()

#define FCITX_UNUSED(X) ((void)(X))

#ifdef __cplusplus
#define FCITX_C_DECL_BEGIN extern "C" {
#define FCITX_C_DECL_END }
#else
#define FCITX_C_DECL_BEGIN
#define FCITX_C_DECL_END
#endif

#define FCITX_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#ifdef __GNUC__
#define _FCITX_UNUSED_ __attribute__((__unused__))
#else
#define _FCITX_UNUSED_
#endif

#define FCITX_WHITE_SPACE "\f\n\r\t\v "

#endif // _FCITX_UTILS_MACROS_H_
