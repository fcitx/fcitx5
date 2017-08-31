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

#include "fcitxutils_export.h"

// steal some Qt macro here

#define FCITX_DECLARE_PRIVATE(Class)                                           \
    inline Class##Private *d_func() {                                          \
        return reinterpret_cast<Class##Private *>(d_ptr.get());                \
    }                                                                          \
    inline const Class##Private *d_func() const {                              \
        return reinterpret_cast<Class##Private *>(d_ptr.get());                \
    }                                                                          \
    friend class Class##Private;

#define FCITX_D() auto *const d = d_func()

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

#define FCITX_WHITESPACE "\f\n\r\t\v "

#define FCITX_EXPAND(x) x

#define FCITX_FOR_EACH_0(...)
#define FCITX_FOR_EACH_1(what, x, ...) what(x)
#define FCITX_FOR_EACH_2(what, x, ...)                                         \
    what(x) FCITX_EXPAND(FCITX_FOR_EACH_1(what, __VA_ARGS__))
#define FCITX_FOR_EACH_3(what, x, ...)                                         \
    what(x) FCITX_EXPAND(FCITX_FOR_EACH_2(what, __VA_ARGS__))
#define FCITX_FOR_EACH_4(what, x, ...)                                         \
    what(x) FCITX_EXPAND(FCITX_FOR_EACH_3(what, __VA_ARGS__))
#define FCITX_FOR_EACH_5(what, x, ...)                                         \
    what(x) FCITX_EXPAND(FCITX_FOR_EACH_4(what, __VA_ARGS__))
#define FCITX_FOR_EACH_6(what, x, ...)                                         \
    what(x) FCITX_EXPAND(FCITX_FOR_EACH_5(what, __VA_ARGS__))
#define FCITX_FOR_EACH_7(what, x, ...)                                         \
    what(x) FCITX_EXPAND(FCITX_FOR_EACH_6(what, __VA_ARGS__))
#define FCITX_FOR_EACH_8(what, x, ...)                                         \
    what(x) FCITX_EXPAND(FCITX_FOR_EACH_7(what, __VA_ARGS__))
#define FCITX_FOR_EACH_9(what, x, ...)                                         \
    what(x) FCITX_EXPAND(FCITX_FOR_EACH_8(what, __VA_ARGS__))
#define FCITX_FOR_EACH_10(what, x, ...)                                        \
    what(x) FCITX_EXPAND(FCITX_FOR_EACH_9(what, __VA_ARGS__))
#define FCITX_FOR_EACH_11(what, x, ...)                                        \
    what(x) FCITX_EXPAND(FCITX_FOR_EACH_10(what, __VA_ARGS__))
#define FCITX_FOR_EACH_12(what, x, ...)                                        \
    what(x) FCITX_EXPAND(FCITX_FOR_EACH_11(what, __VA_ARGS__))

#define FCITX_FOR_EACH_IDX_0(...)
#define FCITX_FOR_EACH_IDX_1(what, x, ...) what(1, x)
#define FCITX_FOR_EACH_IDX_2(what, x, ...)                                     \
    what(2, x) FCITX_EXPAND(FCITX_FOR_EACH_IDX_1(what, __VA_ARGS__))
#define FCITX_FOR_EACH_IDX_3(what, x, ...)                                     \
    what(3, x) FCITX_EXPAND(FCITX_FOR_EACH_IDX_2(what, __VA_ARGS__))
#define FCITX_FOR_EACH_IDX_4(what, x, ...)                                     \
    what(4, x) FCITX_EXPAND(FCITX_FOR_EACH_IDX_3(what, __VA_ARGS__))
#define FCITX_FOR_EACH_IDX_5(what, x, ...)                                     \
    what(5, x) FCITX_EXPAND(FCITX_FOR_EACH_IDX_4(what, __VA_ARGS__))
#define FCITX_FOR_EACH_IDX_6(what, x, ...)                                     \
    what(6, x) FCITX_EXPAND(FCITX_FOR_EACH_IDX_5(what, __VA_ARGS__))
#define FCITX_FOR_EACH_IDX_7(what, x, ...)                                     \
    what(7, x) FCITX_EXPAND(FCITX_FOR_EACH_IDX_6(what, __VA_ARGS__))
#define FCITX_FOR_EACH_IDX_8(what, x, ...)                                     \
    what(8, x) FCITX_EXPAND(FCITX_FOR_EACH_IDX_7(what, __VA_ARGS__))
#define FCITX_FOR_EACH_IDX_9(what, x, ...)                                     \
    what(9, x) FCITX_EXPAND(FCITX_FOR_EACH_IDX_8(what, __VA_ARGS__))
#define FCITX_FOR_EACH_IDX_10(what, x, ...)                                    \
    what(10, x) FCITX_EXPAND(FCITX_FOR_EACH_IDX_9(what, __VA_ARGS__))
#define FCITX_FOR_EACH_IDX_11(what, x, ...)                                    \
    what(11, x) FCITX_EXPAND(FCITX_FOR_EACH_IDX_10(what, __VA_ARGS__))
#define FCITX_FOR_EACH_IDX_12(what, x, ...)                                    \
    what(12, x) FCITX_EXPAND(FCITX_FOR_EACH_IDX_11(what, __VA_ARGS__))

#define FCITX_GET_ARG13(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, N,  \
                        ...)                                                   \
    N

#define FCITX_HAS_COMMA(...)                                                   \
    FCITX_GET_ARG13(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)

#define FCITX_TRIGGER_PARENTHESIS_(...) ,

#define FCITX_IS_EMPTY(...)                                                    \
    FCITX_IS_EMPTY_(FCITX_HAS_COMMA(__VA_ARGS__),                              \
                    FCITX_HAS_COMMA(FCITX_TRIGGER_PARENTHESIS_ __VA_ARGS__),   \
                    FCITX_HAS_COMMA(__VA_ARGS__()),                            \
                    FCITX_HAS_COMMA(FCITX_TRIGGER_PARENTHESIS_ __VA_ARGS__()))

#define FCITX_PASTE5_(_0, _1, _2, _3, _4) _0##_1##_2##_3##_4

#define FCITX_IS_EMPTY_(_0, _1, _2, _3)                                        \
    FCITX_HAS_COMMA(FCITX_PASTE5_(FCITX_IS_EMPTY_CASE_, _0, _1, _2, _3))

#define FCITX_IS_EMPTY_CASE_0001 ,

#define FCITX_EMPTY_1(X) 0
#define FCITX_EMPTY_0(X) X

#define FCITX_NARG(...)                                                        \
    FCITX_NARG_HELPER_(FCITX_IS_EMPTY(__VA_ARGS__), FCITX_NARG_(__VA_ARGS__))
#define FCITX_NARG_HELPER_(B, VAL)                                             \
    FCITX_NARG_HELPER__(FCITX_CONCATENATE(FCITX_EMPTY_, B), VAL)
#define FCITX_NARG_HELPER__(B, VAL) B(VAL)

#define FCITX_NARG_(...) FCITX_NARG__(__VA_ARGS__, FCITX_RSEQ12())
#define FCITX_NARG__(...) FCITX_EXPAND(FCITX_GET_ARG13(__VA_ARGS__))
#define FCITX_RSEQ12() 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define FCITX_CONCATENATE(x, y) x##y
#define FCITX_FOR_EACH_(N, what, ...)                                          \
    FCITX_EXPAND(FCITX_CONCATENATE(FCITX_FOR_EACH_, N)(what, __VA_ARGS__))
#define FCITX_FOR_EACH(what, ...)                                              \
    FCITX_FOR_EACH_(FCITX_NARG(__VA_ARGS__), what, __VA_ARGS__)

#define FCITX_FOR_EACH_IDX_(N, what, ...)                                      \
    FCITX_EXPAND(FCITX_CONCATENATE(FCITX_FOR_EACH_IDX_, N)(what, __VA_ARGS__))
#define FCITX_FOR_EACH_IDX(what, ...)                                          \
    FCITX_FOR_EACH_IDX_(FCITX_NARG(__VA_ARGS__), what, __VA_ARGS__)

#define FCITX_XSTRINGIFY(...) #__VA_ARGS__
#define FCITX_STRINGIFY(...) FCITX_XSTRINGIFY(__VA_ARGS__)
#define FCITX_RETURN_IF(EXPR, VALUE)                                           \
    if ((EXPR)) {                                                              \
        return (VALUE);                                                        \
    }

#define FCITX_DECLARE_PROPERTY(TYPE, GETTER, SETTER)                           \
    std::conditional_t<std::is_class<TYPE>::value, const TYPE &, TYPE>         \
    GETTER() const;                                                            \
    void SETTER(TYPE);

#define FCITX_DEFINE_PROPERTY_PRIVATE(THIS, TYPE, GETTER, SETTER)              \
    std::conditional_t<std::is_class<TYPE>::value, const TYPE &, TYPE>         \
    THIS::GETTER() const {                                                     \
        FCITX_D();                                                             \
        return d->GETTER##_;                                                   \
    }                                                                          \
    void THIS::SETTER(TYPE v) {                                                \
        FCITX_D();                                                             \
        d->GETTER##_ = std::move(v);                                           \
    }

#define FCITX_DECLARE_VIRTUAL_DTOR(TypeName) virtual ~TypeName();

#define FCITX_DECLARE_MOVE(TypeName)                                           \
    TypeName(TypeName &&other) noexcept;                                       \
    TypeName &operator=(TypeName &&other) noexcept;

#define FCITX_DECLARE_COPY(TypeName)                                           \
    TypeName(const TypeName &other);                                           \
    TypeName &operator=(const TypeName &other);

#define FCITX_DECLARE_COPY_AND_MOVE(TypeName)                                  \
    FCITX_DECLARE_COPY(TypeName)                                               \
    FCITX_DECLARE_MOVE(TypeName)

#define FCITX_DECLARE_VIRTUAL_DTOR_COPY_AND_MOVE(TypeName)                     \
    FCITX_DECLARE_VIRTUAL_DTOR(TypeName)                                       \
    FCITX_DECLARE_COPY_AND_MOVE(TypeName)

#define FCITX_DECLARE_VIRTUAL_DTOR_COPY(TypeName)                              \
    FCITX_DECLARE_VIRTUAL_DTOR(TypeName)                                       \
    FCITX_DECLARE_COPY(TypeName)

#define FCITX_DECLARE_VIRTUAL_DTOR_MOVE(TypeName)                              \
    FCITX_DECLARE_VIRTUAL_DTOR(TypeName)                                       \
    FCITX_DECLARE_MOVE(TypeName)

#define FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE_WITH_SPEC(TypeName, Spec)    \
    ~TypeName() = default;                                                     \
    TypeName(TypeName &&other) Spec = default;                                 \
    TypeName &operator=(TypeName &&other) Spec = default;

#define FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE_WITHOUT_SPEC(TypeName)       \
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE_WITH_SPEC(TypeName, )

// try to enforce rule of three-five-zero
#define FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE(TypeName)                    \
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE_WITH_SPEC(TypeName, noexcept)

#define FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(TypeName)                    \
    ~TypeName() = default;                                                     \
    TypeName(const TypeName &other) = default;                                 \
    TypeName &operator=(const TypeName &other) = default;

#define FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE_WITH_SPEC(TypeName,     \
                                                                 Spec)         \
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_MOVE_WITH_SPEC(TypeName, Spec)        \
    TypeName(const TypeName &other) = default;                                 \
    TypeName &operator=(const TypeName &other) = default;

#define FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE_WITHOUT_SPEC(TypeName)  \
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE_WITH_SPEC(TypeName, )

#define FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(TypeName)               \
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE_WITH_SPEC(TypeName, noexcept)

#define FCITX_DEFINE_DEFAULT_MOVE(TypeName)                                    \
    TypeName::TypeName(TypeName &&other) noexcept = default;                   \
    TypeName &TypeName::operator=(TypeName &&other) noexcept = default;

#define FCITX_DEFINE_DEFAULT_COPY(TypeName)                                    \
    TypeName::TypeName(const TypeName &other) = default;                       \
    TypeName &TypeName::operator=(const TypeName &other) = default;

#define FCITX_DEFINE_DEFAULT_DTOR(TypeName) TypeName::~TypeName() = default;

#define FCITX_DEFINE_DPTR_COPY(TypeName)                                       \
    TypeName::TypeName(const TypeName &other)                                  \
        : d_ptr(                                                               \
              std::make_unique<decltype(d_ptr)::element_type>(*other.d_ptr)) { \
    }                                                                          \
    TypeName &TypeName::operator=(const TypeName &other) {                     \
        if (d_ptr) {                                                           \
            *d_ptr = *other.d_ptr;                                             \
        } else {                                                               \
            d_ptr =                                                            \
                std::make_unique<decltype(d_ptr)::element_type>(*other.d_ptr); \
        }                                                                      \
        return *this;                                                          \
    }

#define FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_MOVE(TypeName)                      \
    FCITX_DEFINE_DPTR_COPY(TypeName)                                           \
    FCITX_DEFINE_DEFAULT_MOVE(TypeName)

#define FCITX_DEFINE_DEFAULT_DTOR_AND_MOVE(TypeName)                           \
    FCITX_DEFINE_DEFAULT_DTOR(TypeName)                                        \
    FCITX_DEFINE_DEFAULT_MOVE(TypeName)

#define FCITX_DEFINE_DPTR_COPY_AND_DEFAULT_DTOR_AND_MOVE(TypeName)             \
    FCITX_DEFINE_DPTR_COPY(TypeName)                                           \
    FCITX_DEFINE_DEFAULT_MOVE(TypeName)                                        \
    FCITX_DEFINE_DEFAULT_DTOR(TypeName)

#define FCITX_DEFAULT_DTOR_MOVE_AND_COPY(TypeName)                             \
    FCITX_DEFINE_DEFAULT_COPY(TypeName)                                        \
    FCITX_DEFINE_DEFAULT_MOVE(TypeName)                                        \
    FCITX_DEFINE_DEFAULT_DTOR(TypeName)

namespace fcitx {
template <typename T>
class QPtrHolder {
public:
    explicit QPtrHolder(T *q) : q_ptr(q) {}
    QPtrHolder(const QPtrHolder &) = delete;
    QPtrHolder(QPtrHolder &&) = delete;

    QPtrHolder &operator=(const QPtrHolder &) = delete;
    QPtrHolder &operator=(QPtrHolder &&) = delete;

    T *q_func() { return q_ptr; }
    const T *q_func() const { return q_ptr; }

protected:
    T *q_ptr;
};
}
#endif // _FCITX_UTILS_MACROS_H_
