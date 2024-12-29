/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "variant.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include "../macros.h"
#include "../misc_p.h"
#include "fmessage.h"

namespace fcitx::dbus {

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
    d->types_.emplace(signature, std::move(helper));
}

std::shared_ptr<VariantHelperBase>
VariantTypeRegistry::lookupType(const std::string &signature) const {
    FCITX_D();
    std::shared_lock<std::shared_timed_mutex> lock(d->mutex_);
    const auto *v = findValue(d->types_, signature);
    return v ? *v : nullptr;
}

VariantTypeRegistry &VariantTypeRegistry::defaultRegistry() {
    static VariantTypeRegistry registry;
    return registry;
}

std::shared_ptr<VariantHelperBase>
lookupVariantType(const std::string &signature) {
    return VariantTypeRegistry::defaultRegistry().lookupType(signature);
}

void Variant::writeToMessage(dbus::Message &msg) const {
    helper_->serialize(msg, data_.get());
}

} // namespace fcitx::dbus
