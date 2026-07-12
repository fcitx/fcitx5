/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "tempmodemanager.h"
#include <memory>
#include "fcitx-utils/handlertable.h"
#include "fcitx-utils/macros.h"
#include "event.h"
#include "instance.h"
#include "tempmode.h"
#include "tempmode_p.h"

namespace fcitx {

class TempModeManagerPrivate {
public:
    explicit TempModeManagerPrivate(Instance *instance) : instance_(instance) {
        postInputMethodHandler_ =
            instance->watchEvent<EventType::InputContextKeyEvent>(
                EventWatcherPhase::Default,
                [this](KeyEvent &event) { checkModeTrigger(event); });

        preInputMethodHandler_ =
            instance->watchEvent<EventType::InputContextKeyEvent>(
                EventWatcherPhase::PreInputMethod,
                [this](KeyEvent &event) { keyEvent(event); });

        invokeActionHandler_ =
            instance->watchEvent<EventType::InputContextInvokeAction>(
                EventWatcherPhase::PreInputMethod,
                [this](InvokeActionEvent &event) { invokeAction(event); });

        auto reset = [this](const auto &event) {
            for (auto *mode : modes_.view()) {
                if (mode->isActive(event.inputContext())) {
                    mode->reset(event.inputContext());
                }
            }
        };
        focusOutHandler_ =
            instance->watchEvent<EventType::InputContextFocusOut>(
                EventWatcherPhase::Default, reset);
        resetHandler_ = instance->watchEvent<EventType::InputContextReset>(
            EventWatcherPhase::Default, reset);
        switchInputMethodHandler_ =
            instance->watchEvent<EventType::InputContextSwitchInputMethod>(
                EventWatcherPhase::Default, reset);
    }

    void checkModeTrigger(KeyEvent &keyEvent) const {
        for (auto *mode : modes_.view()) {
            if (mode->triggerTempMode(keyEvent)) {
                keyEvent.filterAndAccept();
                return;
            }
        }
    }

    void keyEvent(KeyEvent &keyEvent) const {
        for (auto *mode : modes_.view()) {
            if (mode->isActive(keyEvent.inputContext())) {
                keyEvent.filter();
                if (mode->keyEvent(keyEvent)) {
                    keyEvent.accept();
                }
                return;
            }
        }
    }

    void invokeAction(InvokeActionEvent &event) const {
        for (auto *mode : modes_.view()) {
            if (mode->isActive(event.inputContext())) {
                event.filter();
                if (mode->invokeAction(event)) {
                    event.accept();
                }
                return;
            }
        }
    }

    Instance *instance_;
    std::unique_ptr<HandlerTableEntry<EventHandler>> postInputMethodHandler_;
    std::unique_ptr<HandlerTableEntry<EventHandler>> preInputMethodHandler_;
    std::unique_ptr<HandlerTableEntry<EventHandler>> invokeActionHandler_;
    std::unique_ptr<HandlerTableEntry<EventHandler>> focusOutHandler_;
    std::unique_ptr<HandlerTableEntry<EventHandler>> resetHandler_;
    std::unique_ptr<HandlerTableEntry<EventHandler>> switchInputMethodHandler_;
    HandlerTable<TempMode *> modes_;
};

TempModeManager::TempModeManager(Instance *instance)
    : d_ptr(std::make_unique<TempModeManagerPrivate>(instance)) {}

TempModeManager::~TempModeManager() {
    FCITX_D();
    for (auto *mode : d->modes_.view()) {
        mode->unregister();
    }
}

void TempModeManager::registerTempMode(TempMode &tempMode) {
    FCITX_D();
    if (tempMode.isRegistered()) {
        return;
    }
    tempMode.d_ptr->registerCallback(d->modes_.add(&tempMode), d->instance_);
}

} // namespace fcitx
