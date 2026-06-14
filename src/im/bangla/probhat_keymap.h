/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef FCITX_BANGLA_PROBHAT_KEYMAP_H
#define FCITX_BANGLA_PROBHAT_KEYMAP_H

#include <cctype>
#include <cstring>

namespace fcitx::bangla {

inline const char *const kProbhatNormal[26] = {
    "া", "ব", "চ", "ড", "ী", "ত", "গ", "হ", "ি", "জ", "ক", "ল", "ম",
    "ন", "ও", "প", "দ", "র", "স", "ট", "ু", "আ", "ূ", "শ", "এ", "য়",
};

inline const char *const kProbhatShift[26] = {
    "অ", "ভ", "ছ", "ঢ", "ঈ", "থ", "ঘ", "ঃ", "ই", "ঝ", "খ", "ং", "ঙ",
    "ণ", "ঔ", "ফ", "ধ", "ড়", "ষ", "ঠ", "উ", "ঋ", "ঊ", "ঢ়", "ঐ", "য",
};

inline const char *probhatLetter(char key, bool shift) {
    if (key >= 'A' && key <= 'Z') {
        shift = true;
        key = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
    } else if (key >= 'a' && key <= 'z') {
        key = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
    } else {
        return nullptr;
    }
    const int idx = key - 'a';
    if (idx < 0 || idx >= 26) {
        return nullptr;
    }
    return shift ? kProbhatShift[idx] : kProbhatNormal[idx];
}

inline const char *probhatPunctuation(char key) {
    switch (key) {
    case ',':
        return ",";
    case '.':
        return "।";
    case '/':
        return "্";
    case ';':
        return ";";
    default:
        return nullptr;
    }
}

} // namespace fcitx::bangla

#endif
