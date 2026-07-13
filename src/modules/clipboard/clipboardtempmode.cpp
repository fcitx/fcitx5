/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "clipboardtempmode.h"
#include <string_view>
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx/candidatelist.h"
#include "fcitx/event.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputpanel.h"
#include "fcitx/instance.h"
#include "fcitx/userinterface.h"
#include "clipboard.h"

namespace fcitx {

void ClipboardState::reset(InputContext *inputContext) {
    inputContext->inputPanel().reset();
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

ClipboardTempMode::ClipboardTempMode(Clipboard *clipboard)
    : clipboard_(clipboard) {}

bool ClipboardTempMode::triggerTempMode(const KeyEvent &keyEvent) {
    if (keyEvent.isRelease()) {
        return false;
    }

    if (keyEvent.key().checkKeyList(*clipboard_->config().triggerKey)) {
        property(keyEvent.inputContext())->setActive(true);
        clipboard_->updateUI(keyEvent.inputContext());
        return true;
    }
    return false;
}

bool ClipboardTempMode::keyEvent(const KeyEvent &keyEvent) {
    auto *inputContext = keyEvent.inputContext();
    if (keyEvent.isRelease()) {
        return false;
    }

    auto candidateList = inputContext->inputPanel().candidateList();
    if (candidateList) {
        int idx = keyEvent.key().digitSelection();
        if (idx >= 0) {
            if (idx < candidateList->size()) {
                candidateList->candidate(idx).select(inputContext);
            }
            return true;
        }
        if (keyEvent.key().check(FcitxKey_space) ||
            keyEvent.key().check(FcitxKey_Return) ||
            keyEvent.key().check(FcitxKey_KP_Enter)) {
            if (!candidateList->empty() && candidateList->cursorIndex() >= 0) {
                candidateList->candidate(candidateList->cursorIndex())
                    .select(inputContext);
            }
            return true;
        }

        if (keyEvent.key().checkKeyList(
                clipboard_->instance()->globalConfig().defaultPrevPage())) {
            auto *pageable = candidateList->toPageable();
            if (!pageable->hasPrev()) {
                if (pageable->usedNextBefore()) {
                    return true;
                }
            } else {
                pageable->prev();
                inputContext->updateUserInterface(
                    UserInterfaceComponent::InputPanel);
                return true;
            }
        }

        if (keyEvent.key().checkKeyList(
                clipboard_->instance()->globalConfig().defaultNextPage())) {
            candidateList->toPageable()->next();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }

        if (keyEvent.key().checkKeyList(clipboard_->instance()
                                            ->globalConfig()
                                            .defaultPrevCandidate())) {
            candidateList->toCursorMovable()->prevCandidate();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }

        if (keyEvent.key().checkKeyList(clipboard_->instance()
                                            ->globalConfig()
                                            .defaultNextCandidate())) {
            candidateList->toCursorMovable()->nextCandidate();
            inputContext->updateUserInterface(
                UserInterfaceComponent::InputPanel);
            return true;
        }
    }

    if (keyEvent.key().isModifier() || keyEvent.key().hasModifier()) {
        return false;
    }
    if (keyEvent.key().check(FcitxKey_Escape)) {
        reset(inputContext);
        return true;
    }
    if (keyEvent.key().check(FcitxKey_Delete) ||
        keyEvent.key().check(FcitxKey_BackSpace)) {
        clipboard_->clear();
        reset(inputContext);
        return true;
    }

    clipboard_->updateUI(inputContext);
    return true;
}

void ClipboardTempMode::reset(InputContext *inputContext) {
    if (auto *clipboardState = property(inputContext)) {
        Base::reset(inputContext);
        clipboardState->reset(inputContext);
    }
}

std::string_view ClipboardTempMode::name() const { return "clipboardState"; }

} // namespace fcitx
