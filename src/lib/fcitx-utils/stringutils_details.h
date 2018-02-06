//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _FCITX_UTILS_STRINGUTILS_DETAIL_H_
#define _FCITX_UTILS_STRINGUTILS_DETAIL_H_

#include "fcitxutils_export.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <tuple>

#ifdef FCITX_STRINGUTILS_ENABLE_BOOST_STRING_VIEW
#include <boost/utility/string_view.hpp>
#endif

namespace fcitx {
namespace stringutils {
namespace details {

template <typename T>
struct UniversalPieceHelper {
    static const T &forward(const T &t) { return t; }
};

template <typename T, size_t TN>
struct UniversalPieceHelper<T[TN]> {
    static std::pair<const char *, std::size_t> forward(const T (&t)[TN]) {
        static_assert(TN > 0, "No char array but only literal");
        return {t, TN - 1};
    }
};

template <typename T>
struct UniversalPieceHelper<T *> {
    static std::pair<const char *, std::size_t> forward(T *t) {
        return {t, std::char_traits<char>::length(t)};
    }
};

#ifdef FCITX_STRINGUTILS_ENABLE_BOOST_STRING_VIEW

template <>
struct UniversalPieceHelper<boost::string_view> {
    static std::pair<const char *, std::size_t> forward(boost::string_view t) {
        return {t.data(), t.size()};
    }
};

#endif

class UniversalPiece {
public:
    UniversalPiece(std::pair<const char *, std::size_t> p)
        : piece_(p.first), size_(p.second) {}

    template <typename T,
              typename = std::enable_if_t<std::is_same<T, char>::value>>
    UniversalPiece(const T *p)
        : piece_(p), size_(std::char_traits<T>::length(p)) {}

    UniversalPiece(const std::string &str)
        : piece_(str.data()), size_(str.size()) {}

    UniversalPiece(char c) = delete;
#define UNIVERSAL_PIECE_NUMERIC_CONVERSION(TYPE, FORMAT_STRING)                \
    UniversalPiece(TYPE i) : piece_(buffer_) {                                 \
        auto size = snprintf(buffer_, sizeof(buffer_), FORMAT_STRING, i);      \
        assert(size >= 0 && static_cast<size_t>(size) + 1 <= sizeof(buffer_)); \
        size_ = size;                                                          \
    }
    UNIVERSAL_PIECE_NUMERIC_CONVERSION(int, "%d");
    UNIVERSAL_PIECE_NUMERIC_CONVERSION(unsigned int, "%u");
    UNIVERSAL_PIECE_NUMERIC_CONVERSION(long, "%ld");
    UNIVERSAL_PIECE_NUMERIC_CONVERSION(unsigned long, "%lu");
    UNIVERSAL_PIECE_NUMERIC_CONVERSION(long long, "%lld");
    UNIVERSAL_PIECE_NUMERIC_CONVERSION(unsigned long long, "%llu");
    UNIVERSAL_PIECE_NUMERIC_CONVERSION(float, "%f");
    UNIVERSAL_PIECE_NUMERIC_CONVERSION(double, "%lf");

    UniversalPiece(const UniversalPiece &) = delete;

    const char *piece() const { return piece_; }
    std::size_t size() const { return size_; }

    std::pair<const char *, std::size_t> toPair() const {
        return {piece_, size_};
    }

    std::pair<const char *, std::size_t>
    toPathPair(const bool removePrefixSlash = true) const {
        auto piece = piece_;
        auto size = size_;
        // Consume prefix and suffix slash.
        if (removePrefixSlash) {
            while (size && piece[0] == '/') {
                ++piece;
                --size;
            }
        }
        while (size && piece[size - 1] == '/') {
            --size;
        }
        // If first component is all slash, keep all of them.
        if (size_ && !removePrefixSlash && !size) {
            return {piece_, size_};
        }

        assert(size > 0);
        return {piece, size};
    }

private:
    const char *piece_;
    std::size_t size_;
    char buffer_[30];
};

FCITXUTILS_EXPORT std::string
concatPieces(std::initializer_list<std::pair<const char *, std::size_t>> list);

FCITXUTILS_EXPORT std::string concatPathPieces(
    std::initializer_list<std::pair<const char *, std::size_t>> list);

} // namespace details

template <typename... Args>
std::string concat(const Args &... args) {
    using namespace ::fcitx::stringutils::details;
    return concatPieces({static_cast<const UniversalPiece &>(
                             details::UniversalPieceHelper<Args>::forward(args))
                             .toPair()...});
}

template <typename FirstArg, typename... Args>
std::string joinPath(const FirstArg &firstArg, const Args &... args) {
    using namespace ::fcitx::stringutils::details;
    return concatPathPieces(
        {static_cast<const UniversalPiece &>(
             UniversalPieceHelper<FirstArg>::forward(firstArg))
             .toPathPair(false),
         static_cast<const UniversalPiece &>(
             UniversalPieceHelper<Args>::forward(args))
             .toPathPair()...});
}

} // namespace stringutils

} // namespace fcitx

#endif // _FCITX_UTILS_STRINGUTILS_DETAIL_H_
