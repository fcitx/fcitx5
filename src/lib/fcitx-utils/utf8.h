/*
 * SPDX-FileCopyrightText: 2015-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_UTF8_H_
#define _FCITX_UTILS_UTF8_H_

/// \addtogroup FcitxUtils
/// \{
/// \file
/// \brief C++ Utility functions for handling utf8 strings.

#include <stdexcept>
#include <string>
#include <fcitx-utils/cutf8.h>
#include <fcitx-utils/misc.h>
#include "fcitxutils_export.h"

namespace fcitx {
namespace utf8 {

/// \brief Return the number UTF-8 characters in the string iterator range.
/// \see lengthValidated()
template <typename Iter>
inline size_t length(Iter start, Iter end) {
    return fcitx_utf8_strnlen(&(*start), std::distance(start, end));
}

/// \brief Return the number UTF-8 characters in the string.
/// \see lengthValidated()
template <typename T>
inline size_t length(const T &s) {
    return length(std::begin(s), std::end(s));
}

/// \brief Return the number UTF-8 characters in the string.
template <typename T>
inline size_t length(const T &s, size_t start, size_t end) {
    return length(std::next(std::begin(s), start),
                  std::next(std::begin(s), end));
}

/// \brief Possible return value of lengthValidated if the string is not valid.
/// \see lengthValidated()
constexpr size_t INVALID_LENGTH = static_cast<size_t>(-1);

/// \brief Validate and return the number UTF-8 characters in the string
/// iterator range
///
/// Will return INVALID_LENGTH if string is not a valid utf8 string.
template <typename Iter>
inline size_t lengthValidated(Iter start, Iter end) {
    return fcitx_utf8_strnlen_validated(&(*start), std::distance(start, end));
}

/// \brief Validate and return the number UTF-8 characters in the string
///
/// Will return INVALID_LENGTH if string is not a valid utf8 string.
template <typename T>
inline size_t lengthValidated(const T &s) {
    return lengthValidated(std::begin(s), std::end(s));
}

/// \brief Check if the string iterator range is valid utf8 string
template <typename Iter>
inline bool validate(Iter start, Iter end) {
    return lengthValidated(start, end) != INVALID_LENGTH;
}

/// \brief Check if the string is valid utf8 string.
template <typename T>
inline bool validate(const T &s) {
    return validate(std::begin(s), std::end(s));
}

/// \brief Convert UCS4 to UTF8 string.
FCITXUTILS_EXPORT std::string UCS4ToUTF8(uint32_t code);

/// \brief Check if a ucs4 is valid.
FCITXUTILS_EXPORT bool UCS4IsValid(uint32_t code);

/// \brief Possible return value for getChar.
constexpr uint32_t INVALID_CHAR = static_cast<uint32_t>(-1);

/// \brief Possible return value for getChar.
constexpr uint32_t NOT_ENOUGH_SPACE = static_cast<uint32_t>(-2);

/// \brief Check the chr value is not two invalid value above.
inline bool isValidChar(uint32_t c) {
    return c != INVALID_CHAR && c != NOT_ENOUGH_SPACE;
}

/// \brief Get next UCS4 char from iter, do not cross end. May return
/// INVALID_CHAR or NOT_ENOUGH_SPACE
template <typename Iter>
inline uint32_t getChar(Iter iter, Iter end) {
    const char *c = &(*iter);
    return fcitx_utf8_get_char_validated(c, std::distance(iter, end), nullptr);
}

/// \brief Get next UCS4 char, may return INVALID_CHAR or NOT_ENOUGH_SPACE
template <typename T>
inline uint32_t getChar(const T &s) {
    return getChar(std::begin(s), std::end(s));
}

template <typename Iter>
inline Iter getNextChar(Iter iter, Iter end, uint32_t *chr) {
    const char *c = &(*iter);
    int plen = 0;
    *chr = fcitx_utf8_get_char_validated(c, std::distance(iter, end), &plen);
    return std::next(iter, plen);
}

/// \brief get the byte length of next N utf-8 character.
///
/// This function has no error check on invalid string or end of string. Check
/// the string before use it.
template <typename Iter>
inline int ncharByteLength(Iter iter, size_t n) {
    const char *c = &(*iter);
    int diff = fcitx_utf8_get_nth_char(c, n) - c;
    return diff;
}

/// \brief Move iter over next n character.
template <typename Iter>
inline Iter nextNChar(Iter iter, size_t n) {
    return std::next(iter, ncharByteLength(iter, n));
}

/// \brief Move iter over next one character.
template <typename Iter>
Iter nextChar(Iter iter) {
    return nextNChar(iter, 1);
}

template <typename Iter>
uint32_t getLastChar(Iter iter, Iter end) {
    uint32_t c = NOT_ENOUGH_SPACE;
    while (iter != end) {
        iter = getNextChar(iter, end, &c);
        if (!isValidChar(c)) {
            break;
        }
    }
    return c;
}

template <typename T>
uint32_t getLastChar(const T &str) {
    return getLastChar(std::begin(str), std::end(str));
}

/// \brief Helper class to iterate character, you need to validate the string
/// before using it.
template <typename Iter>
class UTF8CharIterator {
public:
    typedef std::input_iterator_tag iterator_category;
    typedef uint32_t value_type;
    typedef std::ptrdiff_t difference_type;
    typedef const value_type &reference;
    typedef const value_type *pointer;

    UTF8CharIterator(Iter iter, Iter end) : iter_(iter), end_(end) { update(); }
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(UTF8CharIterator)

    reference operator*() const { return currentChar_; }

    pointer operator->() const { return &currentChar_; }

    std::pair<Iter, Iter> charRange() const { return {iter_, next_}; }

    UTF8CharIterator &operator++() {
        iter_ = next_;
        update();
        return *this;
    }

    UTF8CharIterator operator++(int) {
        auto old = *this;
        ++(*this);
        return old;
    }

    bool operator==(const UTF8CharIterator &other) {
        return iter_ == other.iter_;
    }
    bool operator!=(const UTF8CharIterator &other) {
        return !operator==(other);
    }

private:
    void update() {
        next_ = getNextChar(iter_, end_, &currentChar_);
        if (iter_ != end_ && iter_ == next_) {
            throw std::runtime_error("Invalid UTF8 character.");
        }
    }

    uint32_t currentChar_ = 0;
    Iter iter_;
    Iter next_;
    Iter end_;
};

template <typename Iter>
auto MakeUTF8CharIterator(Iter iter, Iter end) {
    return UTF8CharIterator<Iter>(iter, end);
}

template <typename T>
auto MakeUTF8CharRange(const T &str) {
    return MakeIterRange(MakeUTF8CharIterator(std::begin(str), std::end(str)),
                         MakeUTF8CharIterator(std::end(str), std::end(str)));
}
} // namespace utf8
} // namespace fcitx

#endif // _FCITX_UTILS_UTF8_H_
