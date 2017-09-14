/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _FCITX_UTILS_COMBINETUPLES_H_
#define _FCITX_UTILS_COMBINETUPLES_H_

#include <tuple>

namespace fcitx {

template <typename...>
struct CombineTuples;

template <>
struct CombineTuples<> {
    typedef std::tuple<> type;
};

template <typename _T1, typename... _Rem>
struct CombineTuples<_T1, _Rem...> {
    typedef typename CombineTuples<std::tuple<_T1>, _Rem...>::type type;
};

template <typename... _Ts>
struct CombineTuples<std::tuple<_Ts...>> {
    typedef std::tuple<_Ts...> type;
};

template <typename... _T1s, typename... _T2s, typename... _Rem>
struct CombineTuples<std::tuple<_T1s...>, std::tuple<_T2s...>, _Rem...> {
    typedef typename CombineTuples<std::tuple<_T1s..., _T2s...>, _Rem...>::type
        type;
};

template <typename... Args>
using CombineTuplesType = typename CombineTuples<Args...>::type;

template <int...>
struct Sequence {};

template <int N, int... S>
struct MakeSequence : MakeSequence<N - 1, N - 1, S...> {};

template <int... S>
struct MakeSequence<0, S...> {
    typedef Sequence<S...> type;
};

template <typename... Args, typename F, int... S>
auto callWithIndices(F func, Sequence<S...>, std::tuple<Args...> &tuple) {

    return func(std::get<S>(tuple)...);
}

template <typename... Args, typename F>
auto callWithTuple(F func, std::tuple<Args...> &tuple) {
    typename MakeSequence<sizeof...(Args)>::type a;
    return callWithIndices(func, a, tuple);
}
}

#endif // _FCITX_UTILS_COMBINETUPLES_H_
