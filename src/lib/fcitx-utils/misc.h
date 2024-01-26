/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_MISC_H_
#define _FCITX_UTILS_MISC_H_

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <fcitx-utils/macros.h>
#include "fcitxutils_export.h"

namespace fcitx {

template <class Parent, class Member>
inline std::ptrdiff_t
offsetFromPointerToMember(const Member Parent::*ptr_to_member) {
    const Parent *const parent = 0;
    const char *const member = static_cast<const char *>(
        static_cast<const void *>(&(parent->*ptr_to_member)));
    return std::ptrdiff_t(
        member - static_cast<const char *>(static_cast<const void *>(parent)));
}

template <class Parent, class Member>
inline Parent *parentFromMember(Member *member,
                                const Member Parent::*ptr_to_member) {
    return static_cast<Parent *>(
        static_cast<void *>(static_cast<char *>(static_cast<void *>(member)) -
                            offsetFromPointerToMember(ptr_to_member)));
}

template <class Parent, class Member>
inline const Parent *parentFromMember(const Member *member,
                                      const Member Parent::*ptr_to_member) {
    return static_cast<const Parent *>(static_cast<const void *>(
        static_cast<const char *>(static_cast<const void *>(member)) -
        offsetFromPointerToMember(ptr_to_member)));
}

template <typename Iter>
class KeyIterator {
public:
    using iterator_category = typename Iter::iterator_category;
    using value_type = typename Iter::value_type::first_type;
    using difference_type = typename Iter::difference_type;
    using reference = typename Iter::value_type::first_type &;
    using pointer = typename Iter::value_type::first_type *;

    KeyIterator(Iter iter) : iter_(iter) {}
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_AND_COPY(KeyIterator)

    bool operator==(const KeyIterator &other) const noexcept {
        return iter_ == other.iter_;
    }
    bool operator!=(const KeyIterator &other) const noexcept {
        return !operator==(other);
    }

    KeyIterator &operator++() {
        iter_++;
        return *this;
    }

    KeyIterator operator++(int) {
        auto old = iter_;
        ++(*this);
        return {old};
    }

    reference operator*() { return iter_->first; }

    pointer operator->() { return &iter_->first; }

private:
    Iter iter_;
};

template <typename It>
struct IterRange {
    It begin_, end_;
    It begin() const { return begin_; }
    It end() const { return end_; }
};

template <typename Iter>
IterRange<Iter> MakeIterRange(Iter begin, Iter end) {
    return {begin, end};
}

struct EnumHash {
    template <typename T>
    inline auto operator()(T const value) const {
        return std::hash<std::underlying_type_t<T>>()(
            static_cast<std::underlying_type_t<T>>(value));
    }
};

FCITXUTILS_EXPORT void startProcess(const std::vector<std::string> &args,
                                    const std::string &workingDirectory = {});

FCITXUTILS_EXPORT std::string getProcessName(pid_t pid);

template <auto FreeFunction>
struct FunctionDeleter {
    template <typename T>
    void operator()(T *p) const {
        if (p) {
            FreeFunction(const_cast<std::remove_const_t<T> *>(p));
        }
    }
};
template <typename T, auto FreeFunction = std::free>
using UniqueCPtr = std::unique_ptr<T, FunctionDeleter<FreeFunction>>;
static_assert(
    sizeof(char *) == sizeof(UniqueCPtr<char>),
    "UniqueCPtr size is not same as raw pointer."); // ensure no overhead

using UniqueFilePtr = std::unique_ptr<FILE, FunctionDeleter<std::fclose>>;

template <typename T>
inline auto makeUniqueCPtr(T *ptr) {
    return UniqueCPtr<T>(ptr);
}

FCITXUTILS_EXPORT ssize_t getline(UniqueCPtr<char> &lineptr, size_t *n,
                                  std::FILE *stream);

/**
 * Util function to check whether fcitx is running in flatpak.
 *
 * If environment variable FCITX_OVERRIDE_FLATPAK is true, it will return true.
 * Otherwise it will simply check the existence of file /.flatpak-info .
 *
 * @since 5.0.24
 */
FCITXUTILS_EXPORT bool isInFlatpak();

/**
 * Util function that returns whether it is compile against android.
 *
 * @since 5.1.2
 */
FCITXUTILS_EXPORT constexpr inline bool isAndroid() {
#if defined(ANDROID) || defined(__ANDROID__)
    return true;
#else
    return false;
#endif
}

/**
 * Util function that returns whether it is compile against apple.
 *
 * @since 5.1.7
 */
FCITXUTILS_EXPORT constexpr inline bool isApple() {
#if defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

} // namespace fcitx

#endif // _FCITX_UTILS_MISC_H_
