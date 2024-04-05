/*
 * SPDX-FileCopyrightText: 2024~2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_MODULES_CLIPBOARD_CLIPBOARDENTRY_H_
#define _FCITX5_MODULES_CLIPBOARD_CLIPBOARDENTRY_H_

#include <cstdint>
#include <string>
#include "fcitx-utils/log.h"

namespace fcitx {

struct ClipboardEntry {
    std::string text;
    uint64_t passwordTimestamp = 0;

    bool operator==(const ClipboardEntry &other) const {
        return text == other.text;
    }

    bool operator!=(const ClipboardEntry &other) const {
        return !operator==(other);
    }

    void clear() {
        passwordTimestamp = 0;
        text = std::string();
    }

    bool empty() const { return text.empty(); }
};

inline LogMessageBuilder &operator<<(LogMessageBuilder &builder,
                                     const ClipboardEntry &entry) {
    builder << "ClipboardEntry(" << entry.text
            << ",pass=" << entry.passwordTimestamp << ")";
    return builder;
}

} // namespace fcitx

template <>
struct std::hash<fcitx::ClipboardEntry> {
    size_t operator()(const fcitx::ClipboardEntry &entry) const noexcept {
        return std::hash<std::string>()(entry.text);
    }
};

#endif // _FCITX5_MODULES_CLIPBOARD_CLIPBOARDENTRY_H_