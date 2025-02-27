/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "addoninstance.h"
#include <memory>
#include <stdexcept>
#include <string>
#include "fcitx-utils/macros.h"
#include "fcitx/addoninfo.h"
#include "addoninstance_p.h"

namespace fcitx {

AddonInstance::AddonInstance()
    : d_ptr(std::make_unique<AddonInstancePrivate>()) {}
AddonInstance::~AddonInstance() = default;

void AddonInstance::registerCallback(const std::string &name,
                                     AddonFunctionAdaptorBase *adaptor) {
    FCITX_D();
    d->callbackMap_[name] = adaptor;
}

const AddonInfo *AddonInstance::addonInfo() const {
    FCITX_D();
    return d->addonInfo_;
}

AddonFunctionAdaptorBase *AddonInstance::findCall(const std::string &name) {
    FCITX_D();
    auto iter = d->callbackMap_.find(name);
    if (iter == d->callbackMap_.end()) {
        throw std::runtime_error(name.c_str());
    }
    return iter->second;
}

void AddonInstance::setCanRestart(bool canRestart) {
    FCITX_D();
    d->canRestart_ = canRestart;
}

bool AddonInstance::canRestart() const {
    FCITX_D();
    return d->canRestart_;
}

} // namespace fcitx
