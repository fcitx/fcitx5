/*
 * SPDX-FileCopyrightText: 2016-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "config.h"

#include <pwd.h>
#include <sys/types.h>
#include <fstream>
#include <set>
#include <sstream>
#include <fmt/format.h>
#include "fcitx-config/dbushelper.h"
#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/variant.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputmethodengine.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/misc_p.h"
#include "dbusmodule.h"
#include "keyboard_public.h"
#ifdef ENABLE_X11
#include "xcb_public.h"
#endif

#define FCITX_DBUS_SERVICE "org.fcitx.Fcitx5"
#define FCITX_CONTROLLER_DBUS_INTERFACE "org.fcitx.Fcitx.Controller1"

#define GNOME_HELPER_NAME "org.fcitx.GnomeHelper"
#define GNOME_HELPER_PATH "/org/fcitx/GnomeHelper"
#define GNOME_HELPER_INTERFACE "org.fcitx.GnomeHelper"

using namespace fcitx::dbus;

namespace fcitx {

namespace {
constexpr char globalConfigPath[] = "fcitx://config/global";
constexpr char addonConfigPrefix[] = "fcitx://config/addon/";
constexpr char imConfigPrefix[] = "fcitx://config/inputmethod/";

#ifdef ENABLE_X11
std::string X11GetAddress(AddonInstance *xcb, const std::string &display,
                          xcb_connection_t *conn) {
    static const char selection_prefix[] = "_DBUS_SESSION_BUS_SELECTION_";
    static const char address_prefix[] = "_DBUS_SESSION_BUS_ADDRESS";
    static const char pid_prefix[] = "_DBUS_SESSION_BUS_PID";
    struct passwd *user;

    auto machine = getLocalMachineId();
    if (machine.empty()) {
        return {};
    }

    user = getpwuid(getuid());
    if (!user) {
        return {};
    }
    std::string user_name = user->pw_name;

    auto atom_name =
        stringutils::concat(selection_prefix, user_name, "_", machine);
    auto selectionAtom =
        xcb->call<fcitx::IXCBModule::atom>(display, atom_name, false);
    auto addressAtom =
        xcb->call<fcitx::IXCBModule::atom>(display, address_prefix, false);
    auto pidAtom =
        xcb->call<fcitx::IXCBModule::atom>(display, pid_prefix, false);

    xcb_window_t wid = XCB_WINDOW_NONE;
    {
        auto cookie = xcb_get_selection_owner(conn, selectionAtom);
        auto reply = makeUniqueCPtr(
            xcb_get_selection_owner_reply(conn, cookie, nullptr));
        if (!reply || !reply->owner) {
            return {};
        }
        wid = reply->owner;
    }

    std::string address;
    {
        xcb_get_property_cookie_t get_prop_cookie = xcb_get_property(
            conn, false, wid, addressAtom, XCB_ATOM_STRING, 0, 1024);
        auto reply = makeUniqueCPtr(
            xcb_get_property_reply(conn, get_prop_cookie, nullptr));

        if (!reply || reply->type != XCB_ATOM_STRING ||
            reply->bytes_after > 0 || reply->format != 8) {
            return {};
        }
        auto *data = static_cast<char *>(xcb_get_property_value(reply.get()));
        int length = xcb_get_property_value_length(reply.get());
        auto len = strnlen(&(*data), length);
        address = std::string(&(*data), len);
    }

    if (address.empty()) {
        return {};
    }
    {
        xcb_get_property_cookie_t get_prop_cookie = xcb_get_property(
            conn, false, wid, pidAtom, XCB_ATOM_CARDINAL, 0, sizeof(pid_t));
        auto reply = makeUniqueCPtr(
            xcb_get_property_reply(conn, get_prop_cookie, nullptr));

        if (!reply || reply->type != XCB_ATOM_CARDINAL ||
            reply->bytes_after > 0 || reply->format != 32) {
            return {};
        }
    }

    return address;
}
#endif

} // namespace

class Controller1 : public ObjectVTable<Controller1> {
public:
    Controller1(DBusModule *module, Instance *instance)
        : module_(module), instance_(instance) {}

    void exit() { instance_->exit(); }

    void restart() {
        auto *instance = instance_;
        deferEvent_ = instance_->eventLoop().addDeferEvent(
            [this, instance](EventSource *) {
                instance->restart();
                deferEvent_.reset();
                return false;
            });
    }
    void configure() { instance_->configure(); }
    void configureAddon(const std::string &) { instance_->configure(); }
    void configureInputMethod(const std::string &) { instance_->configure(); }
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
        const auto *group = instance_->inputMethodManager().group(groupName);
        if (group) {
            std::vector<DBusStruct<std::string, std::string>> vec;
            for (const auto &item : group->inputMethodList()) {
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
        if (!imManager.group(groupName)) {
            return;
        }
        InputMethodGroup group(groupName);
        group.setDefaultLayout(defaultLayout);
        for (const auto &entry : entries) {
            group.inputMethodList().push_back(
                InputMethodGroupItem(std::get<0>(entry))
                    .setLayout(std::get<1>(entry)));
        }
        group.setDefaultInputMethod("");
        imManager.setGroup(std::move(group));
        imManager.save();
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
                    layout,
                    [&variants](const std::string &variant,
                                const std::string &description,
                                const std::vector<std::string> &languages) {
                        variants.emplace_back();
                        auto &variantItem = variants.back();
                        std::get<0>(variantItem) = variant;
                        std::get<1>(variantItem) =
                            D_("xkeyboard-config", description);
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

    void switchInputMethodGroup(const std::string &group) {
        instance_->inputMethodManager().setCurrentGroup(group);
    }

    std::string currentInputMethodGroup() {
        return instance_->inputMethodManager().currentGroup().name();
    }

    std::tuple<dbus::Variant, DBusConfig> getConfig(const std::string &uri) {
        std::tuple<dbus::Variant, DBusConfig> result;
        if (uri == globalConfigPath) {
            RawConfig config;
            instance_->globalConfig().save(config);
            std::get<0>(result) = rawConfigToVariant(config);
            std::get<1>(result) =
                dumpDBusConfigDescription(instance_->globalConfig().config());
            return result;
        }
        if (stringutils::startsWith(uri, addonConfigPrefix)) {
            auto addon = uri.substr(sizeof(addonConfigPrefix) - 1);
            auto pos = addon.find('/');
            std::string subPath;
            if (pos != std::string::npos) {
                subPath = addon.substr(pos + 1);
                addon = addon.substr(0, pos);
            }
            if (const auto *addonInfo =
                    instance_->addonManager().addonInfo(addon)) {
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
            auto *addonInstance = instance_->addonManager().addon(addon, true);
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
            }
            throw dbus::MethodCallError("org.freedesktop.DBus.Error.Failed",
                                        "Failed to get addon config.");
        }
        if (stringutils::startsWith(uri, imConfigPrefix)) {
            auto im = uri.substr(sizeof(imConfigPrefix) - 1);
            const auto *entry = instance_->inputMethodManager().entry(im);
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
            auto *engine = instance_->inputMethodEngine(im);
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
            }

            throw dbus::MethodCallError("org.freedesktop.DBus.Error.Failed",
                                        "Failed to get input method.");
        }
        throw dbus::MethodCallError("org.freedesktop.DBus.Error.InvalidArgs",
                                    "Configuration does not exist.");
    }

    void setConfig(const std::string &uri, const dbus::Variant &v) {
        std::tuple<dbus::Variant, DBusConfig> result;
        RawConfig config = variantToRawConfig(v);
        if (uri == globalConfigPath) {
            instance_->globalConfig().load(config, true);
            if (instance_->globalConfig().safeSave()) {
                instance_->reloadConfig();
            }
        } else if (stringutils::startsWith(uri, addonConfigPrefix)) {
            auto addon = uri.substr(sizeof(addonConfigPrefix) - 1);
            auto pos = addon.find('/');
            std::string subPath;
            if (pos != std::string::npos) {
                subPath = addon.substr(pos + 1);
                addon = addon.substr(0, pos);
            }
            auto *addonInstance = instance_->addonManager().addon(addon, true);
            if (addonInstance) {
                FCITX_DEBUG() << "Saving addon config to: " << uri;
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
            const auto *entry = instance_->inputMethodManager().entry(im);
            auto *engine = instance_->inputMethodEngine(im);
            if (entry && engine) {
                FCITX_DEBUG() << "Saving input method config to: " << uri;
                engine->setConfigForInputMethod(*entry, config);
            } else {
                throw dbus::MethodCallError("org.freedesktop.DBus.Error.Failed",
                                            "Failed to get input method.");
            }
        } else {
            throw dbus::MethodCallError(
                "org.freedesktop.DBus.Error.InvalidArgs",
                "Configuration does not exist.");
        }
    }

    std::vector<dbus::DBusStruct<std::string, std::string, std::string, int32_t,
                                 bool, bool>>
    getAddons() {
        std::vector<dbus::DBusStruct<std::string, std::string, std::string,
                                     int32_t, bool, bool>>
            result;
        // Track override.
        const auto &enabled = instance_->globalConfig().enabledAddons();
        std::unordered_set<std::string> enabledSet(enabled.begin(),
                                                   enabled.end());
        const auto &disabled = instance_->globalConfig().disabledAddons();
        std::unordered_set<std::string> disabledSet(disabled.begin(),
                                                    disabled.end());
        for (auto category : {AddonCategory::InputMethod,
                              AddonCategory::Frontend, AddonCategory::Loader,
                              AddonCategory::Module, AddonCategory::UI}) {
            auto names = instance_->addonManager().addonNames(category);
            for (const auto &name : names) {
                const auto *info = instance_->addonManager().addonInfo(name);
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

    std::vector<dbus::DBusStruct<std::string, std::string, std::string, int32_t,
                                 bool, bool, bool, std::vector<std::string>,
                                 std::vector<std::string>>>
    getAddonsV2() {
        std::vector<dbus::DBusStruct<
            std::string, std::string, std::string, int32_t, bool, bool, bool,
            std::vector<std::string>, std::vector<std::string>>>
            result;
        // Track override.
        const auto &enabled = instance_->globalConfig().enabledAddons();
        std::unordered_set<std::string> enabledSet(enabled.begin(),
                                                   enabled.end());
        const auto &disabled = instance_->globalConfig().disabledAddons();
        std::unordered_set<std::string> disabledSet(disabled.begin(),
                                                    disabled.end());
        for (auto category : {AddonCategory::InputMethod,
                              AddonCategory::Frontend, AddonCategory::Loader,
                              AddonCategory::Module, AddonCategory::UI}) {
            auto names = instance_->addonManager().addonNames(category);
            for (const auto &name : names) {
                const auto *info = instance_->addonManager().addonInfo(name);
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
                                    info->isConfigurable(), enabled,
                                    info->onDemand(), info->dependencies(),
                                    info->optionalDependencies());
            }
        }
        return result;
    }

    void setAddonsState(
        const std::vector<dbus::DBusStruct<std::string, bool>> &addons) {
        const auto &enabled = instance_->globalConfig().enabledAddons();
        std::set<std::string> enabledSet(enabled.begin(), enabled.end());

        const auto &disabled = instance_->globalConfig().disabledAddons();
        std::set<std::string> disabledSet(disabled.begin(), disabled.end());
        for (const auto &item : addons) {
            const auto *info =
                instance_->addonManager().addonInfo(std::get<0>(item));
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

    void openX11Connection(const std::string &name) {
#ifdef ENABLE_X11
        if (auto *xcb = module_->xcb()) {
            xcb->call<IXCBModule::openConnection>(name);
            return;
        }
#else
        FCITX_UNUSED(name);
#endif
        throw dbus::MethodCallError("org.freedesktop.DBus.Error.InvalidArgs",
                                    "XCB addon is not available.");
    }

    std::string debugInfo() {
        std::stringstream ss;
        instance_->inputContextManager().foreachGroup([&ss](FocusGroup *group) {
            ss << "Group [" << group->display() << "] has " << group->size()
               << " InputContext(s)" << std::endl;
            group->foreach([&ss](InputContext *ic) {
                ss << "  IC [";
                for (auto v : ic->uuid()) {
                    ss << fmt::format("{:02x}", static_cast<int>(v));
                }
                ss << "] program:" << ic->program()
                   << " frontend:" << ic->frontend()
                   << " cap:" << fmt::format("{:x}", ic->capabilityFlags())
                   << " focus:" << ic->hasFocus() << std::endl;
                return true;
            });
            return true;
        });
        ss << "Input Context without group" << std::endl;
        instance_->inputContextManager().foreach([&ss](InputContext *ic) {
            if (ic->focusGroup()) {
                return true;
            }
            ss << "  IC [";
            for (auto v : ic->uuid()) {
                ss << fmt::format("{:02x}", static_cast<int>(v));
            }
            ss << "] program:" << ic->program()
               << " frontend:" << ic->frontend() << " focus:" << ic->hasFocus()
               << std::endl;
            return true;
        });
        return ss.str();
    }

    void refresh() {
        deferEvent_ =
            instance_->eventLoop().addDeferEvent([this](EventSource *) {
                instance_->refresh();
                deferEvent_.reset();
                return false;
            });
    }

    bool checkUpdate() { return instance_->checkUpdate(); }

private:
    DBusModule *module_;
    Instance *instance_;
    std::unique_ptr<EventSource> deferEvent_;

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
    FCITX_OBJECT_VTABLE_METHOD(switchInputMethodGroup, "SwitchInputMethodGroup",
                               "s", "");
    FCITX_OBJECT_VTABLE_METHOD(currentInputMethodGroup,
                               "CurrentInputMethodGroup", "", "s");
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
    FCITX_OBJECT_VTABLE_METHOD(deactivate, "Deactivate", "", "");
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
    FCITX_OBJECT_VTABLE_METHOD(setConfig, "SetConfig", "sv", "");

    FCITX_OBJECT_VTABLE_METHOD(getAddons, "GetAddons", "", "a(sssibb)");
    FCITX_OBJECT_VTABLE_METHOD(getAddonsV2, "GetAddonsV2", "",
                               "a(sssibbbasas)");
    FCITX_OBJECT_VTABLE_METHOD(setAddonsState, "SetAddonsState", "a(sb)", "");
    FCITX_OBJECT_VTABLE_METHOD(openX11Connection, "OpenX11Connection", "s", "");
    FCITX_OBJECT_VTABLE_METHOD(debugInfo, "DebugInfo", "", "s");
    FCITX_OBJECT_VTABLE_METHOD(refresh, "Refresh", "", "");
    FCITX_OBJECT_VTABLE_METHOD(checkUpdate, "CheckUpdate", "", "b");
};

DBusModule::DBusModule(Instance *instance)
    : instance_(instance), bus_(connectToSessionBus()),
      serviceWatcher_(std::make_unique<dbus::ServiceWatcher>(*bus_)) {
    bus_->attachEventLoop(&instance->eventLoop());
    auto uniqueName = bus_->uniqueName();
    Flags<RequestNameFlag> requestFlag = RequestNameFlag::AllowReplacement;
    if (instance_->willTryReplace()) {
        requestFlag |= RequestNameFlag::ReplaceExisting;
    }
    if (!bus_->requestName(FCITX_DBUS_SERVICE, requestFlag)) {
        instance_->exit();
        throw std::runtime_error("Unable to request dbus name. Is there "
                                 "another fcitx already running?");
    }

    disconnectedSlot_ = bus_->addMatch(
        dbus::MatchRule("org.freedesktop.DBus.Local",
                        "/org/freedesktop/DBus/Local",
                        "org.freedesktop.DBus.Local", "Disconnected"),
        [instance](dbus::Message &) {
            FCITX_INFO() << "Disconnected from DBus, exiting...";
            instance->exit();
            return true;
        });

    selfWatcher_ = serviceWatcher_->watchService(
        FCITX_DBUS_SERVICE,
        [uniqueName, instance](const std::string &, const std::string &,
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

std::unique_ptr<dbus::Bus> DBusModule::connectToSessionBus() {
    try {
        auto bus = std::make_unique<dbus::Bus>(dbus::BusType::Session);
        return bus;
    } catch (...) {
    }
#ifdef ENABLE_X11
    if (auto *xcbAddon = xcb()) {
        std::string address;
        // This callback should be called immediately for all existing X11
        // connection.
        auto callback =
            xcbAddon->call<IXCBModule::addConnectionCreatedCallback>(
                [xcbAddon, &address](const std::string &name,
                                     xcb_connection_t *conn, int,
                                     FocusGroup *) {
                    if (!address.empty()) {
                        return;
                    }
                    address = X11GetAddress(xcbAddon, name, conn);
                });
        FCITX_DEBUG() << "DBus address from X11: " << address;
        if (!address.empty()) {
            return std::make_unique<dbus::Bus>(address);
        }
    }
#endif
    throw std::runtime_error("Failed to connect to session dbus");
}

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

bool DBusModule::hasXkbHelper() const { return !xkbHelperName_.empty(); }

class DBusModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new DBusModule(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::DBusModuleFactory)
