/*
 * SPDX-FileCopyrightText: 2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_UNICODE_LITTLEENDIANSERIALIZATION_H_
#define _FCITX_MODULES_UNICODE_LITTLEENDIANSERIALIZATION_H_

#include <array>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include "fcitx-utils/endian_p.h"

namespace fcitx::unicode {

template <typename T>
    requires(std::is_integral_v<T> &&
             !std::is_same_v<std::remove_cv_t<T>, bool> &&
             (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 ||
              sizeof(T) == 8))
T ReadLittleEndian(const std::byte *data) noexcept {
    using UnsignedT = std::make_unsigned_t<std::remove_cv_t<T>>;
    UnsignedT value;
    std::memcpy(&value, data, sizeof(value));
    if constexpr (sizeof(T) == 1) {
        return static_cast<T>(value);
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T>(le16toh(value));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(le32toh(value));
    } else {
        return static_cast<T>(le64toh(value));
    }
}

template <typename T>
    requires(
        std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool> &&
        (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8))
class LittleEndian {
public:
    T value() const noexcept { return ReadLittleEndian<T>(data_.data()); }

private:
    std::array<std::byte, sizeof(T)> data_;
};

} // namespace fcitx::unicode

#endif // _FCITX_MODULES_UNICODE_LITTLEENDIANSERIALIZATION_H_
