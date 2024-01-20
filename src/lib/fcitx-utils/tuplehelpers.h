/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_COMBINETUPLES_H_
#define _FCITX_UTILS_COMBINETUPLES_H_

#include <tuple>

namespace fcitx {

template <typename...>
struct CombineTuples;

template <>
struct CombineTuples<> {
    using type = std::tuple<>;
};

template <typename _T1, typename... _Rem>
struct CombineTuples<_T1, _Rem...> {
    using type = typename CombineTuples<std::tuple<_T1>, _Rem...>::type;
};

template <typename... _Ts>
struct CombineTuples<std::tuple<_Ts...>> {
    using type = std::tuple<_Ts...>;
};

template <typename... _T1s, typename... _T2s, typename... _Rem>
struct CombineTuples<std::tuple<_T1s...>, std::tuple<_T2s...>, _Rem...> {
    using type =
        typename CombineTuples<std::tuple<_T1s..., _T2s...>, _Rem...>::type;
};

template <typename... Args>
using CombineTuplesType = typename CombineTuples<Args...>::type;

template <int...>
struct Sequence {};

template <int N, int... S>
struct MakeSequence : MakeSequence<N - 1, N - 1, S...> {};

template <int... S>
struct MakeSequence<0, S...> {
    using type = Sequence<S...>;
};

template <typename... Args, typename F, int... S>
auto callWithIndices(const F &func, Sequence<S...>,
                     std::tuple<Args...> &tuple) {

    return func(std::get<S>(tuple)...);
}

template <typename... Args, typename F>
auto callWithTuple(F func, std::tuple<Args...> &tuple) {
    typename MakeSequence<sizeof...(Args)>::type a;
    return callWithIndices(func, a, tuple);
}
} // namespace fcitx

#endif // _FCITX_UTILS_COMBINETUPLES_H_
