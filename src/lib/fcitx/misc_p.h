/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MISC_P_H_
#define _FCITX_MISC_P_H_

#include <cstdlib>
#include <fstream>
#include <functional>
#include <string>
#include <type_traits>
#include <fcitx-utils/charutils.h>
#include <fcitx-utils/environ.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/misc_p.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputmethodentry.h>
#include <fcitx/inputmethodmanager.h>
#include <fcitx/instance.h>

// This ia a file for random private util functions that we'd like to share
// among different modules.
namespace fcitx {

class Finally {
public:
    Finally(std::function<void()> func) : func_(std::move(func)) {}
    ~Finally() { func_(); }

private:
    std::function<void()> func_;
};

static inline std::pair<std::string, std::string>
parseLayout(const std::string &layout) {
    auto pos = layout.find('-');
    if (pos == std::string::npos) {
        return {layout, ""};
    }
    return {layout.substr(0, pos), layout.substr(pos + 1)};
}

using GetCandidateListSizeCallback = std::function<int()>;
using GetCandidateWordCallback = std::function<const CandidateWord &(int idx)>;
static inline const CandidateWord *nthCandidateIgnorePlaceholder(
    GetCandidateListSizeCallback getCandidateListSizeCallback,
    GetCandidateWordCallback getCandidateWordCallback, int idx) {
    int total = 0;
    const int size = getCandidateListSizeCallback();
    if (idx < 0 || idx >= size) {
        return nullptr;
    }
    for (int i = 0; i < size; i++) {
        const auto &candidate = getCandidateWordCallback(i);
        if (candidate.isPlaceHolder()) {
            continue;
        }
        if (idx == total) {
            return &candidate;
        }
        ++total;
    }
    return nullptr;
}

static inline const CandidateWord *
nthCandidateIgnorePlaceholder(const CandidateList &candidateList, int idx) {
    return nthCandidateIgnorePlaceholder(
        [&candidateList]() { return candidateList.size(); },
        [&candidateList](int idx) -> const CandidateWord & {
            return candidateList.candidate(idx);
        },
        idx);
}

static inline std::string readFileContent(const std::string &file) {
    std::ifstream fin(file, std::ios::binary | std::ios::in);
    std::vector<char> buffer;
    constexpr auto chunkSize = 4096;
    do {
        auto curSize = buffer.size();
        buffer.resize(curSize + chunkSize);
        if (!fin.read(buffer.data() + curSize, chunkSize)) {
            buffer.resize(curSize + fin.gcount());
            break;
        }
    } while (0);
    std::string str{buffer.begin(), buffer.end()};
    return stringutils::trim(str);
}

static inline std::string getLocalMachineId(const std::string &fallback = {}) {
    auto content = readFileContent("/var/lib/dbus/machine-id");
    if (content.empty()) {
        content = readFileContent("/etc/machine-id");
    }

    return content.empty() ? fallback : content;
}

// Return false if XDG_SESSION_TYPE is set and is not given type.
static inline bool isSessionType(std::string_view type) {
    auto sessionType = getEnvironment("XDG_SESSION_TYPE");
    if (sessionType && *sessionType != type) {
        return false;
    }
    return true;
}

enum class DesktopType {
    KDE6,
    KDE5,
    KDE4,
    GNOME,
    Cinnamon,
    MATE,
    LXDE,
    XFCE,
    DEEPIN,
    UKUI,
    Sway,
    Unknown
};

static inline DesktopType getDesktopType() {
    std::string desktop;
    // new standard
    if (auto desktopEnv = getEnvironment("XDG_CURRENT_DESKTOP")) {
        desktop = std::move(*desktopEnv);
    }
    if (desktop.empty()) {
        // old standard, guaranteed by display manager.
        if (auto desktopEnv = getEnvironment("DESKTOP_SESSION")) {
            desktop = std::move(*desktopEnv);
        }
    }

    for (auto &c : desktop) {
        c = charutils::tolower(c);
    }
    auto desktops =
        stringutils::split(desktop, ":", stringutils::SplitBehavior::SkipEmpty);
    for (const auto &desktop : desktops) {
        if (desktop == "kde") {
            auto versionInt = 0;
            if (auto version = getEnvironment("KDE_SESSION_VERSION")) {
                try {
                    versionInt = std::stoi(*version);
                } catch (...) {
                }
            }
            if (versionInt == 4) {
                return DesktopType::KDE4;
            }
            if (versionInt == 5) {
                return DesktopType::KDE5;
            }
            return DesktopType::KDE6;
        }
        if (desktop == "x-cinnamon") {
            return DesktopType::Cinnamon;
        }
        if (desktop == "lxde") {
            return DesktopType::LXDE;
        }
        if (desktop == "mate") {
            return DesktopType::MATE;
        }
        if (desktop == "gnome") {
            return DesktopType::GNOME;
        }
        if (desktop == "xfce") {
            return DesktopType::XFCE;
        }
        if (desktop == "deepin") {
            return DesktopType::DEEPIN;
        }
        if (desktop == "ukui") {
            return DesktopType::UKUI;
        }
        if (desktop == "sway") {
            return DesktopType::Sway;
        }
    }
    return DesktopType::Unknown;
}

static inline bool isKDE() {
    static const DesktopType desktop = getDesktopType();
    return desktop == DesktopType::KDE4 || desktop == DesktopType::KDE5;
}

static inline bool hasTwoKeyboardInCurrentGroup(Instance *instance) {
    size_t numOfKeyboard = 0;
    for (const auto &item :
         instance->inputMethodManager().currentGroup().inputMethodList()) {
        if (auto entry = instance->inputMethodManager().entry(item.name());
            entry && entry->isKeyboard()) {
            ++numOfKeyboard;
        }
        if (numOfKeyboard >= 2) {
            return true;
        }
    }

    std::unordered_set<std::string> groupLayouts;
    for (const auto &groupName : instance->inputMethodManager().groups()) {
        if (auto group = instance->inputMethodManager().group(groupName)) {
            groupLayouts.insert(group->defaultLayout());
        }
        if (groupLayouts.size() >= 2) {
            return true;
        }
    }
    return false;
}

static inline std::string getCurrentLanguage() {
    for (const char *vars : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
        auto lang = getEnvironment(vars);
        if (lang && !lang->empty()) {
            return std::move(*lang);
        }
    }
    return "";
}

static inline std::string stripLanguage(const std::string &lc) {
    auto lang = stringutils::trim(lc);
    auto idx = lang.find('.');
    lang = lang.substr(0, idx);
    idx = lc.find('@');
    lang = lang.substr(0, idx);
    if (lang.empty()) {
        return "C";
    }
    return lang;
}

static inline bool isSingleModifier(const Key &key) {
    return key.isModifier() && (key.states() == 0 ||
                                key.states() == Key::keySymToStates(key.sym()));
}
static inline bool isSingleKey(const Key &key) {
    return isSingleModifier(key) || !key.hasModifier();
}

inline void hash_combine(std::size_t &seed, std::size_t value) noexcept {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace fcitx

FCITX_DECLARE_LOG_CATEGORY(keyTrace);

#define FCITX_KEYTRACE() FCITX_LOGC(::keyTrace, Debug)

#endif // _FCITX_MISC_P_H_
