/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "virtualinputcontext.h"
#include <cassert>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include "fcitx-utils/capabilityflags.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx/inputcontext.h"
#include "appmonitor.h"

namespace fcitx {

InputContext *VirtualInputContextGlue::delegatedInputContext() {
    if (virtualICManager_) {
        if (auto *virtualIC = virtualICManager_->focusedVirtualIC()) {
            return virtualIC;
        }
    }
    return this;
}

void VirtualInputContextGlue::focusInWrapper() {
    if (virtualICManager_) {
        virtualICManager_->setRealFocus(true);
    } else {
        focusIn();
    }
}

void VirtualInputContextGlue::focusOutWrapper() {
    if (virtualICManager_) {
        virtualICManager_->setRealFocus(false);
    } else {
        focusOut();
    }
}

void VirtualInputContextGlue::updateSurroundingTextWrapper() {
    updateSurroundingText();
    if (auto *ic = delegatedInputContext(); ic != this) {
        ic->surroundingText() = surroundingText();
        ic->updateSurroundingText();
    }
}

void VirtualInputContextGlue::setCapabilityFlagsWrapper(CapabilityFlags flags) {
    setCapabilityFlags(flags);
    if (auto *ic = delegatedInputContext(); ic != this) {
        ic->setCapabilityFlags(flags);
    }
}

VirtualInputContextManager::VirtualInputContextManager(
    InputContextManager *manager, VirtualInputContextGlue *parent,
    AppMonitor *app)
    : manager_(manager), parentIC_(parent), app_(app) {
    conn_ = app_->appUpdated.connect(
        [this](const std::unordered_map<std::string, std::string> &appState,
               const std::optional<std::string> &focus) {
            appUpdated(appState, focus);
        });
    parent->setVirtualInputContextManager(this);
}

VirtualInputContextManager::~VirtualInputContextManager() {
    parentIC_->setVirtualInputContextManager(nullptr);
}

void VirtualInputContextManager::setRealFocus(bool focus) {
    parentIC_->setRealFocus(focus);
    updateFocus();
}

void VirtualInputContextManager::updateFocus() {
    InputContext *ic = nullptr;
    if (focus_) {
        if (auto *value = findValue(managed_, *focus_)) {
            ic = value->get();
        } else {
            auto result = managed_.emplace(
                *focus_,
                std::make_unique<VirtualInputContext>(
                    *manager_, *findValue(lastAppState_, *focus_), parentIC_));
            assert(result.second);
            ic = result.first->second.get();
        }
    } else {
        ic = parentIC_;
    }
    assert(ic);
    if (parentIC_->realFocus()) {
        // forward capability flags on focus in.
        if (ic != parentIC_) {
            ic->setCapabilityFlags(parentIC_->capabilityFlags());
            ic->surroundingText() = parentIC_->surroundingText();
            ic->updateSurroundingText();
        }
        ic->focusIn();
    } else {
        parentIC_->focusOut();
        for (const auto &[_, ic] : managed_) {
            ic->focusOut();
        }
    }
}

InputContext *VirtualInputContextManager::focusedVirtualIC() {
    if (!focus_) {
        return nullptr;
    }
    auto *inputContext = findValue(managed_, *focus_);
    return inputContext ? inputContext->get() : nullptr;
}

void VirtualInputContextManager::appUpdated(
    const std::unordered_map<std::string, std::string> &appState,
    std::optional<std::string> focus) {
    assert(!focus || appState.count(*focus));
    lastAppState_ = appState;
    for (auto iter = managed_.begin(); iter != managed_.end();) {
        if (!findValue(lastAppState_, iter->first)) {
            iter = managed_.erase(iter);
        } else {
            ++iter;
        }
    }

    focus_ = std::move(focus);
    updateFocus();
}

} // namespace fcitx
