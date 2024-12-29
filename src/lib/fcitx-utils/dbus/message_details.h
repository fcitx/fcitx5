/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_DBUS_MESSAGE_DETAILS_H_
#define _FCITX_UTILS_DBUS_MESSAGE_DETAILS_H_

// IWYU pragma: private, include "message.h"

#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <fcitx-utils/metastring.h>
#include <fcitx-utils/tuplehelpers.h>
#include <fcitx-utils/unixfd.h>

namespace fcitx::dbus {

template <typename T>
struct MakeTupleIfNeeded {
    using type = std::tuple<T>;
};

template <typename... Args>
struct MakeTupleIfNeeded<std::tuple<Args...>> {
    using type = std::tuple<Args...>;
};

template <typename T>
using MakeTupleIfNeededType = typename MakeTupleIfNeeded<T>::type;

template <typename T>
struct RemoveTupleIfUnnecessary {
    using type = T;
};

template <typename Arg>
struct RemoveTupleIfUnnecessary<std::tuple<Arg>> {
    using type = Arg;
};

template <typename T>
using RemoveTupleIfUnnecessaryType = typename RemoveTupleIfUnnecessary<T>::type;

class ObjectPath;
class Variant;

template <typename... Args>
struct DBusStruct;

template <typename Key, typename Value>
class DictEntry;

template <typename T>
struct DBusSignatureTraits;

template <char>
struct DBusSignatureToBasicType;

#define DBUS_SIGNATURE_TRAITS(TYPENAME, SIG)                                   \
    template <>                                                                \
    struct DBusSignatureTraits<TYPENAME> {                                     \
        using signature = MetaString<SIG>;                                     \
    };                                                                         \
                                                                               \
    template <>                                                                \
    struct DBusSignatureToBasicType<SIG> {                                     \
        using type = TYPENAME;                                                 \
    };

DBUS_SIGNATURE_TRAITS(std::string, 's');
DBUS_SIGNATURE_TRAITS(uint8_t, 'y');
DBUS_SIGNATURE_TRAITS(bool, 'b');
DBUS_SIGNATURE_TRAITS(int16_t, 'n');
DBUS_SIGNATURE_TRAITS(uint16_t, 'q');
DBUS_SIGNATURE_TRAITS(int32_t, 'i');
DBUS_SIGNATURE_TRAITS(uint32_t, 'u');
DBUS_SIGNATURE_TRAITS(int64_t, 'x');
DBUS_SIGNATURE_TRAITS(uint64_t, 't');
DBUS_SIGNATURE_TRAITS(double, 'd');
DBUS_SIGNATURE_TRAITS(UnixFD, 'h');
DBUS_SIGNATURE_TRAITS(ObjectPath, 'o');
DBUS_SIGNATURE_TRAITS(Variant, 'v');

template <typename K, typename V>
struct DBusSignatureTraits<std::pair<K, V>> {
    using signature =
        ConcatMetaStringType<typename DBusSignatureTraits<K>::signature,
                             typename DBusSignatureTraits<V>::signature>;
};

template <typename Arg, typename... Args>
struct DBusSignatureTraits<std::tuple<Arg, Args...>> {
    using signature = ConcatMetaStringType<
        typename DBusSignatureTraits<Arg>::signature,
        typename DBusSignatureTraits<std::tuple<Args...>>::signature>;
};

template <>
struct DBusSignatureTraits<std::tuple<>> {
    using signature = MetaString<>;
};

template <typename... Args>
struct DBusSignatureTraits<DBusStruct<Args...>> {
    using signature = ConcatMetaStringType<
        MetaString<'('>,
        typename DBusSignatureTraits<std::tuple<Args...>>::signature,
        MetaString<')'>>;
};

template <typename Key, typename Value>
struct DBusSignatureTraits<DictEntry<Key, Value>> {
    using signature = ConcatMetaStringType<
        MetaString<'{'>,
        typename DBusSignatureTraits<std::tuple<Key, Value>>::signature,
        MetaString<'}'>>;
};

template <typename T>
struct DBusSignatureTraits<std::vector<T>> {
    using signature =
        ConcatMetaStringType<MetaString<'a'>,
                             typename DBusSignatureTraits<T>::signature>;
};

template <typename T>
struct DBusContainerSignatureTraits;

template <typename... Args>
struct DBusContainerSignatureTraits<DBusStruct<Args...>> {
    using signature =
        typename DBusSignatureTraits<std::tuple<Args...>>::signature;
};

template <typename Key, typename Value>
struct DBusContainerSignatureTraits<DictEntry<Key, Value>> {
    using signature =
        typename DBusSignatureTraits<std::tuple<Key, Value>>::signature;
};

template <typename T>
struct DBusContainerSignatureTraits<std::vector<T>> {
    using signature = typename DBusSignatureTraits<T>::signature;
};

template <char left, char right, int level, typename S>
struct SkipTillNext;

template <char left, char right, int level, char first, char... next>
struct SkipTillNext<left, right, level, MetaString<first, next...>> {
    using type =
        typename SkipTillNext<left, right, level, MetaString<next...>>::type;
    using str = ConcatMetaStringType<
        MetaString<first>,
        typename SkipTillNext<left, right, level, MetaString<next...>>::str>;
};

template <char left, char right, int level, char... next>
struct SkipTillNext<left, right, level, MetaString<left, next...>> {
    using type = typename SkipTillNext<left, right, level + 1,
                                       MetaString<next...>>::type;
    using str =
        ConcatMetaStringType<MetaString<left>,
                             typename SkipTillNext<left, right, level + 1,
                                                   MetaString<next...>>::str>;
};

template <char left, char right, int level, char... next>
struct SkipTillNext<left, right, level, MetaString<right, next...>> {
    using type = typename SkipTillNext<left, right, level - 1,
                                       MetaString<next...>>::type;
    using str =
        ConcatMetaStringType<MetaString<right>,
                             typename SkipTillNext<left, right, level - 1,
                                                   MetaString<next...>>::str>;
};

template <char left, char right, char first, char... next>
struct SkipTillNext<left, right, 0, MetaString<first, next...>> {
    using type = MetaString<first, next...>;
    using str = MetaString<>;
};

// This is required to resolve ambiguity like (i)(i), when a '(' appear
// immediate after a closed ')'.
template <char left, char right, char... next>
struct SkipTillNext<left, right, 0, MetaString<left, next...>> {
    using type = MetaString<left, next...>;
    using str = MetaString<>;
};

template <char left, char right>
struct SkipTillNext<left, right, 0, MetaString<>> {
    using type = MetaString<>;
    using str = MetaString<>;
};

template <char... c>
struct DBusSignatureToType;

template <char... c>
DBusSignatureToType<c...>
    DBusMetaStringSignatureToTupleHelper(MetaString<c...>);

template <typename T>
using DBusMetaStringSignatureToTuple = MakeTupleIfNeededType<
    typename decltype(DBusMetaStringSignatureToTupleHelper(
        std::declval<T>()))::type>;

template <typename... Args>
DBusStruct<Args...> TupleToDBusStructHelper(std::tuple<Args...>);

template <typename T>
using TupleToDBusStruct = decltype(TupleToDBusStructHelper(std::declval<T>()));

template <typename Key, typename Value>
DictEntry<Key, Value> TupleToDictEntryHelper(std::tuple<Key, Value>);

template <typename T>
using TupleToDictEntry = decltype(TupleToDictEntryHelper(std::declval<T>()));

template <char... next>
struct DBusSignatureGetNextSignature;

template <char first, char... nextChar>
struct DBusSignatureGetNextSignature<first, nextChar...> {
    using cur = typename DBusSignatureToType<first>::type;
    using next = typename DBusSignatureToType<nextChar...>::type;
};

template <char... nextChar>
struct DBusSignatureGetNextSignature<'a', nextChar...> {
    using SplitType = DBusSignatureGetNextSignature<nextChar...>;
    using cur = std::vector<typename SplitType::cur>;
    using next = typename SplitType::next;
};

template <int level, typename S>
using SkipTillNextParentheses = SkipTillNext<'(', ')', level, S>;

template <int level, typename S>
using SkipTillNextBrace = SkipTillNext<'{', '}', level, S>;

template <char... nextChar>
struct DBusSignatureGetNextSignature<'(', nextChar...> {
    using cur = TupleToDBusStruct<DBusMetaStringSignatureToTuple<
        RemoveMetaStringTailType<typename SkipTillNextParentheses<
            1, MetaString<nextChar...>>::str>>>;
    using next = DBusMetaStringSignatureToTuple<
        typename SkipTillNextParentheses<1, MetaString<nextChar...>>::type>;
};

template <char... nextChar>
struct DBusSignatureGetNextSignature<'{', nextChar...> {
    using cur = TupleToDictEntry<
        DBusMetaStringSignatureToTuple<RemoveMetaStringTailType<
            typename SkipTillNextBrace<1, MetaString<nextChar...>>::str>>>;
    using next = DBusMetaStringSignatureToTuple<
        typename SkipTillNextBrace<1, MetaString<nextChar...>>::type>;
};

template <char... c>
struct DBusSignatureToType {
    using SplitType = DBusSignatureGetNextSignature<c...>;
    using type = RemoveTupleIfUnnecessaryType<
        CombineTuplesType<MakeTupleIfNeededType<typename SplitType::cur>,
                          MakeTupleIfNeededType<typename SplitType::next>>>;
};
template <char c>
struct DBusSignatureToType<c> {
    using type = typename DBusSignatureToBasicType<c>::type;
};

template <>
struct DBusSignatureToType<> {
    using type = std::tuple<>;
};

template <typename M>
struct MetaStringToDBusTuple;

template <char... c>
struct MetaStringToDBusTuple<MetaString<c...>> {
    using type = typename DBusSignatureToType<c...>::type;
};

template <typename M>
using MetaStringToDBusTupleType = typename MetaStringToDBusTuple<M>::type;

template <typename T>
struct DBusTupleToReturn {
    using type = T;
};

template <>
struct DBusTupleToReturn<std::tuple<>> {
    using type = void;
};

template <typename T>
using DBusTupleToReturnType = typename DBusTupleToReturn<T>::type;

#define FCITX_STRING_TO_DBUS_TUPLE(STRING)                                     \
    ::fcitx::dbus::MakeTupleIfNeededType<                                      \
        ::fcitx::dbus::MetaStringToDBusTupleType<fcitxMakeMetaString(STRING)>>
#define FCITX_STRING_TO_DBUS_TYPE(STRING)                                      \
    ::fcitx::dbus::DBusTupleToReturnType<                                      \
        ::fcitx::dbus::MetaStringToDBusTupleType<fcitxMakeMetaString(STRING)>>
} // namespace fcitx::dbus

#endif // _FCITX_UTILS_DBUS_MESSAGE_DETAILS_H_
