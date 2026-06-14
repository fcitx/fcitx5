/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "bangla.h"

#include "probhat_keymap.h"

#include <fcitx-utils/fdstreambuf.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx/inputpanel.h>
#include <fcitx/userinterface.h>

#include <cctype>
#include <istream>
#include <sstream>

namespace fcitx {

bool BanglaEngine::isAvroInputChar(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '`' || ch == '^' ||
           ch == ',' || ch == '.' || (ch >= '0' && ch <= '9');
}

void BanglaEngine::clearPreedit(InputContext *ic) {
    ic->inputPanel().setClientPreedit(Text());
    ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void BanglaEngine::activate(const InputMethodEntry &, InputContextEvent &event) {
    buffer_.clear();
    loadUserDict();
    if (auto *ic = event.inputContext()) {
        clearPreedit(ic);
    }
}

void BanglaEngine::reset(const InputMethodEntry &, InputContextEvent &event) {
    buffer_.clear();
    if (auto *ic = event.inputContext()) {
        clearPreedit(ic);
    }
}

void BanglaEngine::clearBuffer(InputContext *ic) {
    buffer_.clear();
    clearPreedit(ic);
}

void BanglaEngine::updatePreedit(InputContext *ic) {
    if (buffer_.empty()) {
        clearPreedit(ic);
    } else {
        ic->inputPanel().setClientPreedit(Text(parser_.parse(buffer_)));
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }
}

std::string BanglaEngine::resolveCommit() const {
    if (buffer_.empty()) {
        return {};
    }
    if (buffer_.front() == '`') {
        return buffer_.size() > 1 ? buffer_.substr(1) : "`";
    }
    if (const auto it = userDict_.find(buffer_); it != userDict_.end()) {
        return it->second;
    }
    return parser_.parse(buffer_);
}

void BanglaEngine::commitBuffer(InputContext *ic) {
    if (buffer_.empty()) {
        return;
    }
    ic->commitString(resolveCommit());
    clearBuffer(ic);
}

void BanglaEngine::loadUserDict() {
    userDict_.clear();
    const auto file =
        StandardPaths::global().open(StandardPathsType::PkgData, "bangla/user.dict.txt");
    if (!file.isValid()) {
        return;
    }
    IFDStreamBuf buf(file.fd());
    std::istream in(&buf);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto tab = line.find('\t');
        const auto eq = line.find('=');
        const auto sep = tab != std::string::npos ? tab
                         : eq != std::string::npos ? eq
                                                   : std::string::npos;
        if (sep == std::string::npos) {
            continue;
        }
        auto key = line.substr(0, sep);
        auto value = line.substr(sep + 1);
        while (!key.empty() && std::isspace(static_cast<unsigned char>(key.back()))) {
            key.pop_back();
        }
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
            value.erase(value.begin());
        }
        if (!key.empty() && !value.empty()) {
            userDict_.emplace(std::move(key), std::move(value));
        }
    }
}

bool BanglaEngine::isProbhatEntry(const InputMethodEntry &entry) {
    return entry.uniqueName() == "probhat";
}

void BanglaEngine::handleProbhatKeyEvent(KeyEvent &keyEvent) {
    if (keyEvent.isRelease()) {
        return;
    }

    const KeyStates modifiers{KeyState::Ctrl, KeyState::Alt};
    if (keyEvent.key().states() & modifiers) {
        return;
    }

    auto *ic = keyEvent.inputContext();
    const auto sym = keyEvent.key().sym();

    if (sym == FcitxKey_Return) {
        ic->commitString("\n");
        keyEvent.filterAndAccept();
        return;
    }

    if (sym == FcitxKey_space) {
        ic->commitString(" ");
        keyEvent.filterAndAccept();
        return;
    }

    if (!keyEvent.key().isSimple()) {
        return;
    }

    const auto ch = static_cast<char>(sym);
    const bool shift = keyEvent.key().states().test(KeyState::Shift);
    std::string text;
    if (const auto *letter = bangla::probhatLetter(ch, shift)) {
        text = letter;
    } else if (const auto *punct = bangla::probhatPunctuation(ch)) {
        text = punct;
    } else {
        text = Key::keySymToUTF8(sym);
    }

    if (!text.empty()) {
        ic->commitString(text);
        keyEvent.filterAndAccept();
    }
}

void BanglaEngine::keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) {
    if (isProbhatEntry(entry)) {
        handleProbhatKeyEvent(keyEvent);
        return;
    }

    if (keyEvent.isRelease()) {
        return;
    }

    const KeyStates modifiers{KeyState::Ctrl, KeyState::Alt};
    if (keyEvent.key().states() & modifiers) {
        return;
    }

    auto *ic = keyEvent.inputContext();
    const auto sym = keyEvent.key().sym();

    if (sym == FcitxKey_BackSpace) {
        if (buffer_.empty()) {
            return;
        }
        buffer_.pop_back();
        updatePreedit(ic);
        keyEvent.filterAndAccept();
        return;
    }

    if (sym == FcitxKey_Return || sym == FcitxKey_space) {
        if (!buffer_.empty()) {
            const auto commit = resolveCommit();
            ic->commitString(commit + (sym == FcitxKey_space ? " " : "\n"));
            clearBuffer(ic);
        } else {
            ic->commitString(sym == FcitxKey_space ? " " : "\n");
        }
        keyEvent.filterAndAccept();
        return;
    }

    if (keyEvent.key().isSimple()) {
        const auto ch = static_cast<char>(sym);
        if (isAvroInputChar(ch)) {
            buffer_.push_back(ch);
            updatePreedit(ic);
            keyEvent.filterAndAccept();
            return;
        }
        if (!buffer_.empty()) {
            commitBuffer(ic);
            const auto text = Key::keySymToUTF8(sym);
            if (!text.empty()) {
                ic->commitString(text);
            }
            keyEvent.filterAndAccept();
        }
    }
}

} // namespace fcitx

FCITX_ADDON_FACTORY_V2(bangla, fcitx::BanglaFactory);
