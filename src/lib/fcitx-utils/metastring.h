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
#ifndef _FCITX_UTILS_METASTRING_H_
#define _FCITX_UTILS_METASTRING_H_

#include <exception>

namespace fcitx {

template <char... c>
struct MetaString final {
public:
    static constexpr std::size_t size() { return m_size; }
    static constexpr const char *data() { return m_str; }

private:
    static constexpr const char m_str[sizeof...(c) + 1] = {c..., '\0'};
    static const std::size_t m_size = sizeof...(c);
};

template <char... c>
constexpr const char MetaString<c...>::m_str[sizeof...(c) + 1];

template <int N, int M>
constexpr char __getChar(char const(&str)[M]) noexcept {
    return N < M ? str[N] : '\0';
}

template <typename... T>
struct MetaStringCombine;

template <char... c>
struct MetaStringCombine<MetaString<c...>> {
    typedef MetaString<c...> type;
};

template <>
struct MetaStringCombine<MetaString<'\0'>> {
    typedef MetaString<> type;
};

template <char... c, typename... Rem>
struct MetaStringCombine<MetaString<c...>, MetaString<'\0'>, Rem...> {
    typedef typename MetaStringCombine<MetaString<c...>>::type type;
};
template <char... c, char c2, typename... Rem>
struct MetaStringCombine<MetaString<c...>, MetaString<c2>, Rem...> {
    typedef typename MetaStringCombine<MetaString<c..., c2>, Rem...>::type type;
};

template <char... c>
struct MetaStringTrim {
    typedef typename MetaStringCombine<MetaString<c>...>::type type;
};

#define METASTRING_TEMPLATE_16(N, S)                                                                                   \
    ::fcitx::__getChar<0x##N##0>(S), ::fcitx::__getChar<0x##N##1>(S), ::fcitx::__getChar<0x##N##2>(S),                 \
        ::fcitx::__getChar<0x##N##3>(S), ::fcitx::__getChar<0x##N##4>(S), ::fcitx::__getChar<0x##N##5>(S),             \
        ::fcitx::__getChar<0x##N##6>(S), ::fcitx::__getChar<0x##N##7>(S), ::fcitx::__getChar<0x##N##8>(S),             \
        ::fcitx::__getChar<0x##N##9>(S), ::fcitx::__getChar<0x##N##A>(S), ::fcitx::__getChar<0x##N##B>(S),             \
        ::fcitx::__getChar<0x##N##C>(S), ::fcitx::__getChar<0x##N##D>(S), ::fcitx::__getChar<0x##N##E>(S),             \
        ::fcitx::__getChar<0x##N##F>(S)

#define METASTRING_TEMPLATE_256(N, S)                                                                                  \
    METASTRING_TEMPLATE_16(N##0, S), METASTRING_TEMPLATE_16(N##1, S), METASTRING_TEMPLATE_16(N##2, S), METASTRING_TEMPLATE_16(N##3, S),\
    METASTRING_TEMPLATE_16(N##4, S), METASTRING_TEMPLATE_16(N##5, S), METASTRING_TEMPLATE_16(N##6, S), METASTRING_TEMPLATE_16(N##7, S),\
    METASTRING_TEMPLATE_16(N##8, S), METASTRING_TEMPLATE_16(N##9, S), METASTRING_TEMPLATE_16(N##A, S), METASTRING_TEMPLATE_16(N##B, S),\
    METASTRING_TEMPLATE_16(N##C, S), METASTRING_TEMPLATE_16(N##D, S), METASTRING_TEMPLATE_16(N##E, S), METASTRING_TEMPLATE_16(N##F, S)

#define makeMetaString(STRING) ::fcitx::MetaStringTrim<METASTRING_TEMPLATE_256(, STRING)>::type

#ifndef MSTR
#define MSTR(STRING) makeMetaString(STRING)
#endif
}

#endif // _FCITX_UTILS_METASTRING_H_
