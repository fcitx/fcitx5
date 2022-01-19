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
#include <string>
#include <type_traits>
#include "fcitx-utils/charutils.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx/candidatelist.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/instance.h"

// This ia a file for random private util functions that we'd like to share
// among different modules.
namespace fcitx {

static inline std::pair<std::string, std::string>
parseLayout(const std::string &layout) {
    auto pos = layout.find('-');
    if (pos == std::string::npos) {
        return {layout, ""};
    }
    return {layout.substr(0, pos), layout.substr(pos + 1)};
}

static inline const CandidateWord *
nthCandidateIgnorePlaceholder(const CandidateList &candidateList, int idx) {
    int total = 0;
    if (idx < 0 || idx >= candidateList.size()) {
        return nullptr;
    }
    for (int i = 0, e = candidateList.size(); i < e; i++) {
        const auto &candidate = candidateList.candidate(i);
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
    const char *sessionType = getenv("XDG_SESSION_TYPE");
    if (sessionType && std::string_view(sessionType) != type) {
        return false;
    }
    return true;
}

enum class DesktopType {
    KDE5,
    KDE4,
    GNOME,
    Cinnamon,
    MATE,
    LXDE,
    XFCE,
    Unknown
};

static inline DesktopType getDesktopType() {
    std::string desktop;
    auto *desktopEnv = getenv("XDG_CURRENT_DESKTOP");
    if (desktopEnv) {
        desktop = desktopEnv;
    }

    for (auto &c : desktop) {
        c = charutils::tolower(c);
    }
    auto desktops =
        stringutils::split(desktop, ":", stringutils::SplitBehavior::SkipEmpty);
    for (const auto &desktop : desktops) {
        if (desktop == "kde") {
            auto *version = getenv("KDE_SESSION_VERSION");
            auto versionInt = 0;
            if (version) {
                try {
                    versionInt = std::stoi(version);
                } catch (...) {
                }
            }
            if (versionInt == 4) {
                return DesktopType::KDE4;
            }
            return DesktopType::KDE5;
        } else if (desktop == "x-cinnamon") {
            return DesktopType::Cinnamon;
        } else if (desktop == "lxde") {
            return DesktopType::LXDE;
        } else if (desktop == "mate") {
            return DesktopType::MATE;
        } else if (desktop == "gnome") {
            return DesktopType::GNOME;
        } else if (desktop == "xfce") {
            return DesktopType::XFCE;
        }
    }
    return DesktopType::Unknown;
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

} // namespace fcitx

FCITX_DECLARE_LOG_CATEGORY(keyTrace);

#define FCITX_KEYTRACE() FCITX_LOGC(::keyTrace, Debug)

#endif // _FCITX_MISC_P_H_
