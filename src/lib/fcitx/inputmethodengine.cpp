/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "inputmethodengine.h"
#include "inputcontext.h"
#include "inputpanel.h"

namespace fcitx {

std::string InputMethodEngine::subModeIcon(const InputMethodEntry &entry,
                                           InputContext &ic) {
    if (auto *this2 = dynamic_cast<InputMethodEngineV2 *>(this)) {
        return this2->subModeIconImpl(entry, ic);
    }
    return overrideIcon(entry);
}

std::string InputMethodEngine::subModeLabel(const InputMethodEntry &entry,
                                            InputContext &ic) {
    if (auto *this2 = dynamic_cast<InputMethodEngineV2 *>(this)) {
        return this2->subModeLabelImpl(entry, ic);
    }
    return {};
}

void defaultInvokeActionBehavior(InvokeActionEvent &event) {
    auto ic = event.inputContext();
    auto commit = ic->inputPanel().clientPreedit().toStringForCommit();
    if (!commit.empty()) {
        ic->commitString(commit);
    }
    ic->reset(ResetReason::Client);
    event.filter();
}

void InputMethodEngine::invokeAction(const InputMethodEntry &entry,

                                     InvokeActionEvent &event) {
    if (auto *this3 = dynamic_cast<InputMethodEngineV3 *>(this)) {
        return this3->invokeActionImpl(entry, event);
    }
    defaultInvokeActionBehavior(event);
}

void InputMethodEngineV3::invokeActionImpl(const InputMethodEntry &entry,
                                           InvokeActionEvent &event) {
    FCITX_UNUSED(entry);
    defaultInvokeActionBehavior(event);
}

} // namespace fcitx
