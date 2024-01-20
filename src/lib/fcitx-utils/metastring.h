/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_METASTRING_H_
#define _FCITX_UTILS_METASTRING_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief Static string based on template argument.

#include <cstddef>

namespace fcitx {

template <char... c>
struct MetaString final {
public:
    using array_type = char const (&)[sizeof...(c) + 1];

    static constexpr std::size_t size() { return size_; }
    static constexpr const char *data() { return str_; }
    static constexpr bool empty() { return size_ == 0; }

    static constexpr array_type str() { return str_; }

private:
    static constexpr const char str_[sizeof...(c) + 1] = {c..., '\0'};
    static const std::size_t size_ = sizeof...(c);
};

template <int N, int M>
constexpr char __getChar(char const (&str)[M]) noexcept {
    if constexpr (N < M) {
        return str[N];
    }
    return '\0';
}

template <typename... T>
struct MetaStringCombine;

template <char... c>
struct MetaStringCombine<MetaString<c...>> {
    using type = MetaString<c...>;
};

template <>
struct MetaStringCombine<MetaString<'\0'>> {
    using type = MetaString<>;
};

template <char... c, typename... Rem>
struct MetaStringCombine<MetaString<c...>, MetaString<'\0'>, Rem...> {
    using type = typename MetaStringCombine<MetaString<c...>>::type;
};
template <char... c, char c2, typename... Rem>
struct MetaStringCombine<MetaString<c...>, MetaString<c2>, Rem...> {
    using type = typename MetaStringCombine<MetaString<c..., c2>, Rem...>::type;
};

template <typename...>
struct ConcatMetaString;

template <>
struct ConcatMetaString<MetaString<>> {
    using type = MetaString<>;
};

template <char... c>
struct ConcatMetaString<MetaString<c...>> {
    using type = MetaString<c...>;
};

template <char... c1s, char... c2s, typename... _Rem>
struct ConcatMetaString<MetaString<c1s...>, MetaString<c2s...>, _Rem...> {
    using type =
        typename ConcatMetaString<MetaString<c1s..., c2s...>, _Rem...>::type;
};

template <typename... Args>
using ConcatMetaStringType = typename ConcatMetaString<Args...>::type;

template <typename T>
struct RemoveMetaStringTail;
template <typename T>
using RemoveMetaStringTailType = typename RemoveMetaStringTail<T>::type;

template <char first, char... next>
struct RemoveMetaStringTail<MetaString<first, next...>> {
    using type =
        ConcatMetaStringType<MetaString<first>,
                             RemoveMetaStringTailType<MetaString<next...>>>;
};
template <char first>
struct RemoveMetaStringTail<MetaString<first>> {
    using type = MetaString<>;
};

template <typename... T>
struct MetaStringBasenameHelper;

template <typename... T>
using MetaStringBasenameHelperType =
    typename MetaStringBasenameHelper<T...>::type;

template <>
struct MetaStringBasenameHelper<> {
    using type = MetaString<>;
};

template <char... c>
struct MetaStringBasenameHelper<MetaString<c...>> {
    using type = MetaString<c...>;
};

template <char... c>
struct MetaStringBasenameHelper<MetaString<'/', c...>> {
    using type = MetaStringBasenameHelperType<MetaString<c...>>;
};

template <char... c, char c2, typename... Rem>
struct MetaStringBasenameHelper<MetaString<c...>, MetaString<c2>, Rem...> {
    using type = MetaStringBasenameHelperType<MetaString<c..., c2>, Rem...>;
};

template <char... c, typename... Rem>
struct MetaStringBasenameHelper<MetaString<c...>, MetaString<'/'>, Rem...> {
    using type = MetaStringBasenameHelperType<Rem...>;
};

template <typename T>
struct MetaStringBasename;

template <char... c>
struct MetaStringBasename<MetaString<c...>> {
    using type = MetaStringBasenameHelperType<MetaString<c>...>;
};

template <typename T>
using MetaStringBasenameType = typename MetaStringBasename<T>::type;

template <char... c>
struct MetaStringTrim {
    using type = typename MetaStringCombine<MetaString<c>...>::type;
};

template <char... c>
using MetaStringTrimType = typename MetaStringTrim<c...>::type;

#define FCITX_METASTRING_TEMPLATE_16(N, S)                                     \
    ::fcitx::__getChar<0x##N##0>(S), ::fcitx::__getChar<0x##N##1>(S),          \
        ::fcitx::__getChar<0x##N##2>(S), ::fcitx::__getChar<0x##N##3>(S),      \
        ::fcitx::__getChar<0x##N##4>(S), ::fcitx::__getChar<0x##N##5>(S),      \
        ::fcitx::__getChar<0x##N##6>(S), ::fcitx::__getChar<0x##N##7>(S),      \
        ::fcitx::__getChar<0x##N##8>(S), ::fcitx::__getChar<0x##N##9>(S),      \
        ::fcitx::__getChar<0x##N##A>(S), ::fcitx::__getChar<0x##N##B>(S),      \
        ::fcitx::__getChar<0x##N##C>(S), ::fcitx::__getChar<0x##N##D>(S),      \
        ::fcitx::__getChar<0x##N##E>(S), ::fcitx::__getChar<0x##N##F>(S)

#define FCITX_METASTRING_TEMPLATE_256(N, S)                                    \
    FCITX_METASTRING_TEMPLATE_16(N##0, S)                                      \
    , FCITX_METASTRING_TEMPLATE_16(N##1, S),                                   \
        FCITX_METASTRING_TEMPLATE_16(N##2, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##3, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##4, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##5, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##6, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##7, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##8, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##9, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##A, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##B, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##C, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##D, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##E, S),                                 \
        FCITX_METASTRING_TEMPLATE_16(N##F, S)

/// \brief Create meta string from string literal.
#define fcitxMakeMetaString(STRING)                                            \
    ::fcitx::MetaStringTrimType<FCITX_METASTRING_TEMPLATE_256(, STRING)>
} // namespace fcitx

#endif // _FCITX_UTILS_METASTRING_H_
