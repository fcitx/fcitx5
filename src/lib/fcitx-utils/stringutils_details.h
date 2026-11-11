/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_STRINGUTILS_DETAIL_H_
#define _FCITX_UTILS_STRINGUTILS_DETAIL_H_

// IWYU pragma: private, include <fcitx-utils/stringutils.h>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <fcitx-utils/fcitxutils_export.h>
#include <ranges>

namespace fcitx::stringutils::details {

template <bool KeepEmpty>
class SplitView : public std::ranges::view_interface<SplitView<KeepEmpty>> {
public:
    class Iterator {
    public:
        using value_type = std::string_view;
        using difference_type = std::ptrdiff_t;
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::input_iterator_tag;

        constexpr Iterator() = default;

        constexpr std::string_view operator*() const {
            assert(tokenLen_ != std::string_view::npos);
            return rest_.substr(0, tokenLen_);
        }

        constexpr Iterator &operator++() {
            assert(tokenLen_ != std::string_view::npos);
            if (tokenLen_ == rest_.size()) {
                rest_ = {};
                tokenLen_ = std::string_view::npos;
            } else {
                rest_.remove_prefix(tokenLen_ + 1);
                advance();
            }
            return *this;
        }

        constexpr Iterator operator++(int) {
            auto previous = *this;
            ++*this;
            return previous;
        }

        friend constexpr bool operator==(const Iterator &a, const Iterator &b) {
            return a.tokenLen_ == b.tokenLen_ &&
                   a.rest_.data() == b.rest_.data() &&
                   a.rest_.size() == b.rest_.size();
        }

        friend constexpr bool operator==(const Iterator &it,
                                         std::default_sentinel_t /*unused*/) {
            return it.tokenLen_ == std::string_view::npos;
        }

    private:
        friend class SplitView;

        constexpr Iterator(std::string_view str, std::string_view delim)
            : rest_(str), delim_(delim) {
            advance();
        }

        constexpr void advance() {
            if constexpr (!KeepEmpty) {
                const auto start = rest_.find_first_not_of(delim_);
                if (start == std::string_view::npos) {
                    rest_ = {};
                    tokenLen_ = std::string_view::npos;
                    return;
                }
                rest_.remove_prefix(start);
            }
            // When KeepEmpty is true and str is initially empty, end is npos
            // and tokenLen_ becomes 0, yielding a single empty string_view
            // before reaching sentinel on ++ (matching split() behavior).
            const auto end = rest_.find_first_of(delim_);
            tokenLen_ = end == std::string_view::npos ? rest_.size() : end;
        }

        std::string_view rest_;
        std::string_view delim_;
        std::size_t tokenLen_ = std::string_view::npos;
    };

    constexpr SplitView(std::string_view str, std::string_view delim)
        : first_(str, delim) {}

    constexpr Iterator begin() const noexcept { return first_; }
    constexpr std::default_sentinel_t end() const noexcept { return {}; }

private:
    Iterator first_;
};

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

template <>
struct UniversalPieceHelper<std::string_view> {
    static std::pair<const char *, std::size_t> forward(std::string_view t) {
        return {t.data(), t.size()};
    }
};

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
        const auto *piece = piece_;
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
    static constexpr int IntegerBufferSize = 30;
    const char *piece_;
    std::size_t size_;
    char buffer_[IntegerBufferSize];
};

FCITXUTILS_EXPORT std::string
concatPieces(std::initializer_list<std::pair<const char *, std::size_t>> list);

FCITXUTILS_EXPORT std::string concatPathPieces(
    std::initializer_list<std::pair<const char *, std::size_t>> list);

} // namespace fcitx::stringutils::details

template <bool KeepEmpty>
inline constexpr bool std::ranges::enable_borrowed_range<
    fcitx::stringutils::details::SplitView<KeepEmpty>> = true;

#endif // _FCITX_UTILS_STRINGUTILS_DETAIL_H_
