/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "addoninstance.h"

namespace fcitx {

class AddonInstancePrivate {
public:
    std::unordered_map<std::string, AddonFunctionAdaptorBase *> callbackMap_;
};

AddonInstance::AddonInstance()
    : d_ptr(std::make_unique<AddonInstancePrivate>()) {}
AddonInstance::~AddonInstance() = default;

void AddonInstance::registerCallback(const std::string &name,
                                     AddonFunctionAdaptorBase *adaptor) {
    FCITX_D();
    d->callbackMap_[name] = adaptor;
}

AddonFunctionAdaptorBase *AddonInstance::findCall(const std::string &name) {
    FCITX_D();
    auto iter = d->callbackMap_.find(name);
    if (iter == d->callbackMap_.end()) {
        throw std::runtime_error(name.c_str());
    }
    return iter->second;
}
} // namespace fcitx
