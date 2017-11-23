/*
 * Copyright (C) 2016~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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

#include "dbusmodule.h"
#include "fcitx-config/dbushelper.h"
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/variant.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputmethodengine.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "keyboard_public.h"
#include <set>

#define FCITX_DBUS_SERVICE "org.fcitx.Fcitx5"
#define FCITX_CONTROLLER_DBUS_INTERFACE "org.fcitx.Fcitx.Controller1"

#define GNOME_HELPER_NAME "org.fcitx.GnomeHelper"
#define GNOME_HELPER_PATH "/org/fcitx/GnomeHelper"
#define GNOME_HELPER_INTERFACE "org.fcitx.GnomeHelper"

using namespace fcitx::dbus;

namespace fcitx {

namespace {
constexpr char addonConfigPrefix[] = "fcitx://config/addon/";
constexpr char imConfigPrefix[] = "fcitx://config/inputmethod/";
} // namespace

class Controller1 : public ObjectVTable<Controller1> {
public:
    Controller1(DBusModule *module, Instance *instance)
        : module_(module), instance_(instance) {}

    void exit() { instance_->exit(); }

    void restart() {
        auto instance = instance_;
        deferEvent_ = instance_->eventLoop().addDeferEvent(
            [this, instance](EventSource *) {
                instance->restart();
                deferEvent_.reset();
                return false;
            });
    }
    void configure() { instance_->configure(); }
    void configureAddon(const std::string &addon) {
        instance_->configureAddon(addon);
    }
    void configureInputMethod(const std::string &imName) {
        instance_->configureInputMethod(imName);
    }
    std::string currentUI() { return instance_->currentUI(); }
    std::string addonForInputMethod(const std::string &imName) {
        return instance_->addonForInputMethod(imName);
    }
    void activate() { return instance_->activate(); }
    void deactivate() { return instance_->deactivate(); }
    void toggle() { return instance_->toggle(); }
    void resetInputMethodList() { return instance_->resetInputMethodList(); }
    int state() { return instance_->state(); }
    void reloadConfig() { return instance_->reloadConfig(); }
    void reloadAddonConfig(const std::string &addonName) {
        return instance_->reloadAddonConfig(addonName);
    }
    std::string currentInputMethod() { return instance_->currentInputMethod(); }
    void setCurrentInputMethod(const std::string &imName) {
        return instance_->setCurrentInputMethod(imName);
    }

    std::vector<std::string> inputMethodGroups() {
        return instance_->inputMethodManager().groups();
    }

    std::tuple<std::string, std::vector<DBusStruct<std::string, std::string>>>
    inputMethodGroupInfo(const std::string &groupName) {
        auto group = instance_->inputMethodManager().group(groupName);
        if (group) {
            std::vector<DBusStruct<std::string, std::string>> vec;
            for (auto &item : group->inputMethodList()) {
                vec.emplace_back(
                    std::forward_as_tuple(item.name(), item.layout()));
            }
            return {group->defaultLayout(), vec};
        }
        return {"", {}};
    }

    std::vector<DBusStruct<std::string, std::string, std::string, std::string,
                           std::string, std::string, bool>>
    availableInputMethods() {
        std::vector<DBusStruct<std::string, std::string, std::string,
                               std::string, std::string, std::string, bool>>
            entries;
        instance_->inputMethodManager().foreachEntries(
            [&entries](const InputMethodEntry &entry) {
                entries.emplace_back(std::forward_as_tuple(
                    entry.uniqueName(), entry.name(), entry.nativeName(),
                    entry.icon(), entry.label(), entry.languageCode(),
                    entry.isConfigurable()));
                return true;
            });
        return entries;
    }

    void setInputMethodGroupInfo(
        const std::string &groupName, const std::string &defaultLayout,
        const std::vector<DBusStruct<std::string, std::string>> &entries) {
        auto &imManager = instance_->inputMethodManager();
        if (imManager.group(groupName)) {
            InputMethodGroup group(groupName);
            group.setDefaultLayout(defaultLayout);
            for (auto &entry : entries) {
                group.inputMethodList().push_back(
                    InputMethodGroupItem(std::get<0>(entry))
                        .setLayout(std::get<1>(entry)));
            }
            group.setDefaultInputMethod("");
            imManager.setGroup(std::move(group));
        }
    }

    std::vector<DBusStruct<std::string, std::string, std::vector<std::string>,
                           std::vector<DBusStruct<std::string, std::string,
                                                  std::vector<std::string>>>>>
    availableKeyboardLayouts() {
        std::vector<
            DBusStruct<std::string, std::string, std::vector<std::string>,
                       std::vector<DBusStruct<std::string, std::string,
                                              std::vector<std::string>>>>>
            result;
        module_->keyboard()->call<IKeyboardEngine::foreachLayout>(
            [&result, this](const std::string &layout,
                            const std::string &description,
                            const std::vector<std::string> &languages) {
                result.emplace_back();
                auto &layoutItem = result.back();
                std::get<0>(layoutItem) = layout;
                std::get<1>(layoutItem) = D_("xkeyboard-config", description);
                std::get<2>(layoutItem) = languages;
                auto &variants = std::get<3>(layoutItem);
                module_->keyboard()->call<IKeyboardEngine::foreachVariant>(
                    layout, [&variants,
                             this](const std::string &variant,
                                   const std::string &description,
                                   const std::vector<std::string> &languages) {
                        variants.emplace_back();
                        auto &variantItem = variants.back();
                        std::get<0>(variantItem) = variant;
                        std::get<1>(variantItem) =
                            D_("xkeyboard-config", description);
                        ;
                        std::get<2>(variantItem) = languages;
                        return true;
                    });
                return true;
            });
        return result;
    }

    void addInputMethodGroup(const std::string &group) {
        instance_->inputMethodManager().addEmptyGroup(group);
    }

    void removeInputMethodGroup(const std::string &group) {
        instance_->inputMethodManager().removeGroup(group);
    }

    std::tuple<dbus::Variant, DBusConfig> getConfig(const std::string &uri) {
        std::tuple<dbus::Variant, DBusConfig> result;
        if (uri == "fcitx://config/global") {
            RawConfig config;
            instance_->globalConfig().save(config);
            std::get<0>(result) = rawConfigToVariant(config);
            std::get<1>(result) =
                dumpDBusConfigDescription(instance_->globalConfig().config());
            return result;
        } else if (stringutils::startsWith(uri, addonConfigPrefix)) {
            FCITX_INFO() << uri;
            auto addon = uri.substr(sizeof(addonConfigPrefix) - 1);
            auto pos = addon.find('/');
            std::string subPath;
            if (pos != std::string::npos) {
                subPath = addon.substr(pos + 1);
                addon = addon.substr(0, pos);
            }
            if (auto addonInfo = instance_->addonManager().addonInfo(addon)) {
                if (!addonInfo->isConfigurable()) {
                    throw dbus::MethodCallError(
                        "org.freedesktop.DBus.Error.InvalidArgs",
                        "Addon is not configurable.");
                }
            } else {
                throw dbus::MethodCallError(
                    "org.freedesktop.DBus.Error.InvalidArgs",
                    "Addon does not exist.");
            }
            auto addonInstance = instance_->addonManager().addon(addon, true);
            const Configuration *config = nullptr;
            if (addonInstance) {
                if (subPath.empty()) {
                    config = addonInstance->getConfig();
                } else {
                    config = addonInstance->getSubConfig(subPath);
                }
            }
            if (config) {
                RawConfig rawConfig;
                config->save(rawConfig);
                std::get<0>(result) = rawConfigToVariant(rawConfig);
                std::get<1>(result) = dumpDBusConfigDescription(*config);
                return result;
            } else {
                throw dbus::MethodCallError("org.freedesktop.DBus.Error.Failed",
                                            "Failed to get addon config.");
            }
        } else if (stringutils::startsWith(uri, imConfigPrefix)) {
            auto im = uri.substr(sizeof(imConfigPrefix) - 1);
            auto entry = instance_->inputMethodManager().entry(im);
            if (entry) {
                if (!entry->isConfigurable()) {
                    throw dbus::MethodCallError(
                        "org.freedesktop.DBus.Error.InvalidArgs",
                        "Input Method is not configurable.");
                }
            } else {
                throw dbus::MethodCallError(
                    "org.freedesktop.DBus.Error.InvalidArgs",
                    "Input Method does not exist.");
            }
            auto engine = instance_->inputMethodEngine(im);
            const Configuration *config = nullptr;
            if (engine) {
                config = engine->getConfigForInputMethod(*entry);
            }

            if (config) {
                RawConfig rawConfig;
                config->save(rawConfig);
                std::get<0>(result) = rawConfigToVariant(rawConfig);
                std::get<1>(result) = dumpDBusConfigDescription(*config);
                return result;
            } else {
                throw dbus::MethodCallError("org.freedesktop.DBus.Error.Failed",
                                            "Failed to get input method.");
            }
        }
        throw dbus::MethodCallError("org.freedesktop.DBus.Error.InvalidArgs",
                                    "Configuration does not exist.");
    }

    void setConfig(const std::string &uri, const dbus::Variant &v) {
        std::tuple<dbus::Variant, DBusConfig> result;
        FCITX_INFO() << v;
        RawConfig config = variantToRawConfig(v);
        if (uri == "fcitx://config/global") {
            instance_->globalConfig().load(config, true);
            instance_->globalConfig().safeSave();
        } else if (stringutils::startsWith(uri, addonConfigPrefix)) {
            auto addon = uri.substr(sizeof(addonConfigPrefix) - 1);
            auto pos = addon.find('/');
            std::string subPath;
            if (pos != std::string::npos) {
                subPath = addon.substr(pos + 1);
                addon = addon.substr(0, pos);
            }
            auto addonInstance = instance_->addonManager().addon(addon, true);
            if (addonInstance) {
                if (subPath.empty()) {
                    addonInstance->setConfig(config);
                } else {
                    addonInstance->setSubConfig(subPath, config);
                }
            } else {
                throw dbus::MethodCallError("org.freedesktop.DBus.Error.Failed",
                                            "Failed to get addon.");
            }
        } else if (stringutils::startsWith(uri, imConfigPrefix)) {
            auto im = uri.substr(sizeof(imConfigPrefix) - 1);
            auto entry = instance_->inputMethodManager().entry(im);
            auto engine = instance_->inputMethodEngine(im);
            if (entry && engine) {
                engine->setConfigForInputMethod(*entry, config);
            } else {
                throw dbus::MethodCallError("org.freedesktop.DBus.Error.Failed",
                                            "Failed to get input method.");
            }
        }
        throw dbus::MethodCallError("org.freedesktop.DBus.Error.InvalidArgs",
                                    "Configuration does not exist.");
    }

    std::vector<dbus::DBusStruct<std::string, std::string, std::string, int32_t,
                                 bool, bool>>
    getAddons() {
        std::vector<dbus::DBusStruct<std::string, std::string, std::string,
                                     int32_t, bool, bool>>
            result;
        // Track override.
        auto &enabled = instance_->globalConfig().enabledAddons();
        std::unordered_set<std::string> enabledSet(enabled.begin(),
                                                   enabled.end());
        auto &disabled = instance_->globalConfig().disabledAddons();
        std::unordered_set<std::string> disabledSet(disabled.begin(),
                                                    disabled.end());
        for (auto category : {AddonCategory::InputMethod,
                              AddonCategory::Frontend, AddonCategory::Loader,
                              AddonCategory::Module, AddonCategory::UI}) {
            auto names = instance_->addonManager().addonNames(category);
            for (const auto &name : names) {
                auto info = instance_->addonManager().addonInfo(name);
                if (!info) {
                    continue;
                }
                bool enabled = info->isDefaultEnabled();
                if (disabledSet.count(info->uniqueName())) {
                    enabled = false;
                } else if (enabledSet.count(info->uniqueName())) {
                    enabled = true;
                }
                result.emplace_back(info->uniqueName(), info->name().match(),
                                    info->comment().match(),
                                    static_cast<int32_t>(info->category()),
                                    info->isConfigurable(), enabled);
            }
        }
        return result;
    }

    void setAddonsState(
        const std::vector<dbus::DBusStruct<std::string, bool>> &addons) {
        auto &enabled = instance_->globalConfig().enabledAddons();
        std::set<std::string> enabledSet(enabled.begin(), enabled.end());

        auto &disabled = instance_->globalConfig().disabledAddons();
        std::set<std::string> disabledSet(disabled.begin(), disabled.end());
        for (auto &item : addons) {
            auto info = instance_->addonManager().addonInfo(std::get<0>(item));
            if (!info) {
                continue;
            }

            if (std::get<1>(item) == info->isDefaultEnabled()) {
                enabledSet.erase(info->uniqueName());
                disabledSet.erase(info->uniqueName());
            } else if (std::get<1>(item)) {
                enabledSet.insert(info->uniqueName());
                disabledSet.erase(info->uniqueName());
            } else {
                disabledSet.insert(info->uniqueName());
                enabledSet.erase(info->uniqueName());
            }
        }
        instance_->globalConfig().setEnabledAddons(
            {enabledSet.begin(), enabledSet.end()});
        instance_->globalConfig().setDisabledAddons(
            {disabledSet.begin(), disabledSet.end()});
        instance_->globalConfig().safeSave();
    }

private:
    DBusModule *module_;
    Instance *instance_;
    std::unique_ptr<EventSource> deferEvent_;

private:
    FCITX_OBJECT_VTABLE_SIGNAL(inputMethodGroupChanged,
                               "InputMethodGroupsChanged", "");

    FCITX_OBJECT_VTABLE_METHOD(availableKeyboardLayouts,
                               "AvailableKeyboardLayouts", "",
                               "a(ssasa(ssas))");

    FCITX_OBJECT_VTABLE_METHOD(setInputMethodGroupInfo,
                               "SetInputMethodGroupInfo", "ssa(ss)", "");
    FCITX_OBJECT_VTABLE_METHOD(addInputMethodGroup, "AddInputMethodGroup", "s",
                               "");
    FCITX_OBJECT_VTABLE_METHOD(removeInputMethodGroup, "RemoveInputMethodGroup",
                               "s", "");
    FCITX_OBJECT_VTABLE_METHOD(availableInputMethods, "AvailableInputMethods",
                               "", "a(ssssssb)");
    FCITX_OBJECT_VTABLE_METHOD(inputMethodGroupInfo, "InputMethodGroupInfo",
                               "s", "sa(ss)");
    FCITX_OBJECT_VTABLE_METHOD(inputMethodGroups, "InputMethodGroups", "",
                               "as");
    FCITX_OBJECT_VTABLE_METHOD(exit, "Exit", "", "");
    FCITX_OBJECT_VTABLE_METHOD(restart, "Restart", "", "");
    FCITX_OBJECT_VTABLE_METHOD(configure, "Configure", "", "");
    FCITX_OBJECT_VTABLE_METHOD(configureAddon, "ConfigureAddon", "s", "");
    FCITX_OBJECT_VTABLE_METHOD(configureInputMethod, "ConfigureIM", "s", "");
    FCITX_OBJECT_VTABLE_METHOD(currentUI, "CurrentUI", "", "s");
    FCITX_OBJECT_VTABLE_METHOD(addonForInputMethod, "AddonForIM", "s", "s");
    FCITX_OBJECT_VTABLE_METHOD(activate, "Activate", "", "");
    FCITX_OBJECT_VTABLE_METHOD(toggle, "Toggle", "", "");
    FCITX_OBJECT_VTABLE_METHOD(resetInputMethodList, "ResetIMList", "", "");
    FCITX_OBJECT_VTABLE_METHOD(state, "State", "", "i");
    FCITX_OBJECT_VTABLE_METHOD(reloadConfig, "ReloadConfig", "", "");
    FCITX_OBJECT_VTABLE_METHOD(reloadAddonConfig, "ReloadAddonConfig", "s", "");
    FCITX_OBJECT_VTABLE_METHOD(currentInputMethod, "CurrentInputMethod", "",
                               "s");
    FCITX_OBJECT_VTABLE_METHOD(setCurrentInputMethod, "SetCurrentIM", "s", "");
    FCITX_OBJECT_VTABLE_METHOD(getConfig, "GetConfig", "s",
                               "va(sa(sssva{sv}))");
    // FCITX_OBJECT_VTABLE_METHOD(setConfig, "SetConfig", "sv", "");

    ::fcitx::dbus::ObjectVTableMethod setConfigMethod{
        this, "SetConfig", "sv", "", [this](::fcitx::dbus::Message msg) {
            this->setCurrentMessage(&msg);
            FCITX_STRING_TO_DBUS_TUPLE("sv") args;
            msg >> args;
            auto func = [this](auto that, auto &&... args) {
                return that->setConfig(std::forward<decltype(args)>(args)...);
            };
            auto argsWithThis =
                std::tuple_cat(std::make_tuple(this), std::move(args));
            typedef decltype(callWithTuple(func, argsWithThis)) ReturnType;
            ::fcitx::dbus::ReturnValueHelper<ReturnType> helper;
            auto functor = [&argsWithThis, func]() {
                return callWithTuple(func, argsWithThis);
            };
            try {
                helper.call(functor);
                auto reply = msg.createReply();
                static_assert(std::is_same<FCITX_STRING_TO_DBUS_TYPE(""),
                                           ReturnType>::value,
                              "Return type does not match: "
                              "");
                reply << helper.ret;
                reply.send();
            } catch (const ::fcitx::dbus::MethodCallError &error) {
                auto reply = msg.createError(error.name(), error.what());
                reply.send();
            }
            return true;
        }};

    FCITX_OBJECT_VTABLE_METHOD(getAddons, "GetAddons", "", "a(sssibb)");
    FCITX_OBJECT_VTABLE_METHOD(setAddonsState, "SetAddonsState", "a(sb)", "");
};

DBusModule::DBusModule(Instance *instance)
    : bus_(std::make_unique<dbus::Bus>(dbus::BusType::Session)),
      serviceWatcher_(std::make_unique<dbus::ServiceWatcher>(*bus_)),
      instance_(instance) {
    bus_->attachEventLoop(&instance->eventLoop());
    auto uniqueName = bus_->uniqueName();
    if (!bus_->requestName(
            FCITX_DBUS_SERVICE,
            Flags<RequestNameFlag>{RequestNameFlag::AllowReplacement,
                                   RequestNameFlag::ReplaceExisting})) {
        throw std::runtime_error("Unable to request dbus name");
    }

    selfWatcher_ = serviceWatcher_->watchService(
        FCITX_DBUS_SERVICE,
        [this, uniqueName, instance](const std::string &, const std::string &,
                                     const std::string &newName) {
            if (newName != uniqueName) {
                instance->exit();
            }
        });

    xkbWatcher_ = serviceWatcher_->watchService(
        GNOME_HELPER_NAME,
        [this](const std::string &, const std::string &,
               const std::string &newName) { xkbHelperName_ = newName; });

    controller_ = std::make_unique<Controller1>(this, instance);
    bus_->addObjectVTable("/controller", FCITX_CONTROLLER_DBUS_INTERFACE,
                          *controller_);
}

DBusModule::~DBusModule() {}

dbus::Bus *DBusModule::bus() { return bus_.get(); }

bool DBusModule::lockGroup(int group) {
    if (xkbHelperName_.empty()) {
        return false;
    }

    auto msg = bus_->createMethodCall(xkbHelperName_.c_str(), GNOME_HELPER_PATH,
                                      GNOME_HELPER_INTERFACE, "LockXkbGroup");
    msg << group;
    return msg.send();
}

class DBusModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new DBusModule(manager->instance());
    }
};
}

FCITX_ADDON_FACTORY(fcitx::DBusModuleFactory)
