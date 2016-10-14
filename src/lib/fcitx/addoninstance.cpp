/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "addoninstance.h"

namespace fcitx {

class AddonInstancePrivate {
public:
    std::unordered_map<std::string, AddonFunctionAdaptorBase *> callbackMap_;
};

AddonInstance::AddonInstance() : d_ptr(std::make_unique<AddonInstancePrivate>()) {}
AddonInstance::~AddonInstance() {}
void AddonInstance::reloadConfig() {}

void AddonInstance::registerCallback(const std::string &name, AddonFunctionAdaptorBase *adaptor) {
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
}
