/*
 * SPDX-FileCopyrightText: 2020~2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "imselector.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/tempmodemanager.h"

namespace fcitx {

IMSelector::IMSelector(Instance *instance)
    : instance_(instance), tempMode_(this) {
    // Select input method via hotkey.
    eventHandlers_.emplace_back(
        instance_->watchEvent<EventType::InputContextKeyEvent>(
            EventWatcherPhase::PreInputMethod, [this](KeyEvent &keyEvent) {
            auto *inputContext = keyEvent.inputContext();
            if (int index =
                    keyEvent.key().keyListIndex(config_.switchKey.value());
                index >= 0 &&
                selectInputMethod(inputContext, index, /*local=*/false)) {
                keyEvent.filterAndAccept();
                return;
            }
            if (int index =
                    keyEvent.key().keyListIndex(config_.switchKeyLocal.value());
                index >= 0 &&
                selectInputMethod(inputContext, index, /*local=*/true)) {
                keyEvent.filterAndAccept();
                return;
            }
            }));

    std::array<KeySym, 10> syms = {
        FcitxKey_1, FcitxKey_2, FcitxKey_3, FcitxKey_4, FcitxKey_5,
        FcitxKey_6, FcitxKey_7, FcitxKey_8, FcitxKey_9, FcitxKey_0,
    };
    for (auto sym : syms) {
        selectionKeys_.emplace_back(sym);
    }

    instance_->tempModeManager().registerTempMode(tempMode_);
    reloadConfig();
}

void IMSelector::   reloadConfig() { readAsIni(config_, "conf/imselector.conf"); }

void IMSelector::reset(InputContext *inputContext) {
    tempMode_.reset(inputContext);
}

bool IMSelector::selectInputMethod(InputContext *inputContext, size_t index,
                                   bool local) {
    auto &inputMethodManager = instance_->inputMethodManager();
    const auto &list = inputMethodManager.currentGroup().inputMethodList();
    if (index >= list.size()) {
        return false;
    }
    const auto *entry = inputMethodManager.entry(list[index].name());
    if (!entry) {
        return false;
    }
    instance_->setCurrentInputMethod(inputContext, entry->uniqueName(), local);
    reset(inputContext);
    instance_->showInputMethodInformation(inputContext);
    return true;
}

class IMSelectorFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new IMSelector(manager->instance());
    }
};

} // namespace fcitx

FCITX_ADDON_FACTORY_V2(imselector, fcitx::IMSelectorFactory);
