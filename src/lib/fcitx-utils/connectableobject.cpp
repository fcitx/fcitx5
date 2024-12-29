/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "connectableobject.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include "macros.h"
#include "signals.h"

namespace fcitx {

class ConnectableObjectPrivate {
public:
    ConnectableObjectPrivate() = default;
    std::unordered_map<std::string, std::unique_ptr<fcitx::SignalBase>>
        signals_;
    bool destroyed_ = false;
    std::unique_ptr<SignalAdaptor<ConnectableObject::Destroyed>> adaptor_;
};

ConnectableObject::ConnectableObject()
    : d_ptr(std::make_unique<ConnectableObjectPrivate>()) {
    FCITX_D();
    d->adaptor_ = std::make_unique<decltype(d->adaptor_)::element_type>(this);
}

ConnectableObject::~ConnectableObject() { destroy(); }

void ConnectableObject::_registerSignal(
    std::string name, std::unique_ptr<fcitx::SignalBase> signal) {
    FCITX_D();
    d->signals_.emplace(std::move(name), std::move(signal));
}
void ConnectableObject::_unregisterSignal(const std::string &name) {
    FCITX_D();
    d->signals_.erase(name);
}

SignalBase *ConnectableObject::findSignal(const std::string &name) {
    return std::as_const(*this).findSignal(name);
}

SignalBase *ConnectableObject::findSignal(const std::string &name) const {
    FCITX_D();
    auto iter = d->signals_.find(name);
    if (iter != d->signals_.end()) {
        return iter->second.get();
    }
    return nullptr;
}

void ConnectableObject::destroy() {
    FCITX_D();
    if (!d->destroyed_) {
        emit<ConnectableObject::Destroyed>(this);
        disconnectAll<ConnectableObject::Destroyed>();
        d->adaptor_.reset();
        d->destroyed_ = true;
    }
}
} // namespace fcitx
