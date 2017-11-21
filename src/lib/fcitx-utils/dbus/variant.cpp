//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include "variant.h"
#include "fcitx/misc_p.h"
#include <shared_mutex>

namespace fcitx {
namespace dbus {

class VariantTypeRegistryPrivate {
public:
    std::unordered_map<std::string, std::shared_ptr<VariantHelperBase>> types_;
    mutable std::shared_timed_mutex mutex_;
};

VariantTypeRegistry::VariantTypeRegistry()
    : d_ptr(std::make_unique<VariantTypeRegistryPrivate>()) {
    registerType<std::string>();
    registerType<uint8_t>();
    registerType<bool>();
    registerType<int16_t>();
    registerType<uint16_t>();
    registerType<int32_t>();
    registerType<uint32_t>();
    registerType<int64_t>();
    registerType<uint64_t>();
    // registerType<UnixFD>();
    registerType<FCITX_STRING_TO_DBUS_TYPE("a{sv}")>();
    registerType<FCITX_STRING_TO_DBUS_TYPE("as")>();
    registerType<ObjectPath>();
    registerType<Variant>();
}

void VariantTypeRegistry::registerTypeImpl(
    const std::string &signature, std::shared_ptr<VariantHelperBase> helper) {
    FCITX_D();
    std::lock_guard<std::shared_timed_mutex> lock(d->mutex_);
    if (d->types_.count(signature)) {
        return;
    }
    d->types_.emplace(signature, helper);
}

std::shared_ptr<VariantHelperBase>
VariantTypeRegistry::lookupType(const std::string &signature) const {
    FCITX_D();
    std::shared_lock<std::shared_timed_mutex> lock(d->mutex_);
    auto v = findValue(d->types_, signature);
    return v ? *v : nullptr;
}

VariantTypeRegistry &VariantTypeRegistry::defaultRegistry() {
    static VariantTypeRegistry registry;
    return registry;
}

void Variant::writeToMessage(dbus::Message &msg) const {
    helper_->serialize(msg, data_.get());
}

} // namespace dbus
} // namespace fcitx
