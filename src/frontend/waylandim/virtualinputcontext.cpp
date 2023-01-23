/*
 * SPDX-FileCopyrightText: 2022~2022 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "virtualinputcontext.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx/focusgroup.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"

namespace fcitx {

VirtualInputContextManager::VirtualInputContextManager(
    InputContextManager *manager, VirtualInputContextGlue *parent,
    AppMonitorType *app)
    : manager_(manager), parentIC_(parent), app_(app) {
    conn_ = app_->appUpdated.connect(
        [this](const std::unordered_map<AppKey, std::string> &appState,
               const std::optional<AppKey> &focus) {
            appUpdated(appState, focus);
        });
    manager_->instance().
}

void fcitx::VirtualInputContextManager::setRealFocus(bool focus) {
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
    auto *ic = findValue(managed_, *focus_);
    return ic ? ic->get() : nullptr;
}

void fcitx::VirtualInputContextManager::appUpdated(
    const std::unordered_map<AppKey, std::string> &appState,
    std::optional<AppKey> focus) {
    assert(!focus || appState.count(*focus));
    FCITX_INFO() << "UPDATE APP STATE: " << appState << focus;
    lastAppState_ = appState;
    for (auto iter = managed_.begin(); iter != managed_.end();) {
        if (!findValue(lastAppState_, iter->first)) {
            iter = managed_.erase(iter);
        } else {
            ++iter;
        }
    }

    focus_ = focus;
    updateFocus();
}

} // namespace fcitx
