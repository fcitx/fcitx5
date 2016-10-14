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

#include "connectableobject.h"

namespace fcitx {

class ConnectableObjectPrivate {
public:
    std::unordered_map<std::string, std::unique_ptr<fcitx::SignalBase>> signals_;
};

ConnectableObject::ConnectableObject() : d_ptr(std::make_unique<ConnectableObjectPrivate>()) {}

ConnectableObject::~ConnectableObject() {}

void ConnectableObject::_registerSignal(std::string name, std::unique_ptr<fcitx::SignalBase> signal) {
    FCITX_D();
    d->signals_.emplace(std::move(name), std::move(signal));
}
void ConnectableObject::_unregisterSignal(const std::string &name) {
    FCITX_D();
    d->signals_.erase(name);
}

SignalBase *ConnectableObject::findSignal(const std::string &name) {
    FCITX_D();
    auto iter = d->signals_.find(name);
    if (iter != d->signals_.end()) {
        return iter->second.get();
    }
    return nullptr;
}
}
