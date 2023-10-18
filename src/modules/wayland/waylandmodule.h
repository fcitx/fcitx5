/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_WAYLAND_WAYLANDMODULE_H_
#define _FCITX_MODULES_WAYLAND_WAYLANDMODULE_H_

#include <cstdint>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/instance.h"
#include "display.h"
#include "wayland_public.h"
#include "wl_keyboard.h"
#include "wl_seat.h"

namespace fcitx {

class WaylandModule;
class WaylandEventReader;

FCITX_CONFIGURATION(
    WaylandConfig,
    Option<bool> allowOverrideXKB{
        this, "Allow Overriding System XKB Settings",
        _("Allow Overriding System XKB Settings (Only support KDE 5)"), true};);

class WaylandKeyboard {
public:
    WaylandKeyboard(wayland::WlSeat *seat) {
        capConn_ = seat->capabilities().connect([this, seat](uint32_t caps) {
            if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !keyboard_) {
                keyboard_.reset(seat->getKeyboard());
                init();
            } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && keyboard_) {
                keyboard_.reset();
            }
        });
    }

    auto &updateKeymap() { return updateKeymap_; }

private:
    void init() {
        keyboard_->keymap().connect([this](uint32_t, int32_t fd, uint32_t) {
            close(fd);
            updateKeymap_();
        });
    }
    ScopedConnection capConn_;
    std::unique_ptr<wayland::WlKeyboard> keyboard_;
    Signal<void()> updateKeymap_;
};

class WaylandConnection {
public:
    WaylandConnection(WaylandModule *wayland, std::string name);
    WaylandConnection(WaylandModule *wayland, std::string name, int fd);
    ~WaylandConnection();

    const std::string &name() const { return name_; }
    wayland::Display *display() const { return display_.get(); }
    FocusGroup *focusGroup() const { return group_.get(); }
    auto *parent() const { return parent_; }

private:
    void init(wl_display *display);
    void finish();
    void setupKeyboard(wayland::WlSeat *seat);

    WaylandModule *parent_;
    std::string name_;
    // order matters, callback in ioEvent_ uses display_.
    std::unique_ptr<wayland::Display> display_;
    std::unique_ptr<WaylandEventReader> eventReader_;
    std::unique_ptr<FocusGroup> group_;
    int error_ = 0;
    ScopedConnection panelConn_, panelRemovedConn_;
    std::unordered_map<wayland::WlSeat *, std::unique_ptr<WaylandKeyboard>>
        keyboards_;
};

class WaylandModule : public AddonInstance {
public:
    WaylandModule(Instance *instance);
    Instance *instance() { return instance_; }

    bool openConnection(const std::string &name);
    bool openConnectionSocket(int fd);
    void removeConnection(const std::string &name);

    std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>>
    addConnectionCreatedCallback(WaylandConnectionCreated callback);
    std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>>
    addConnectionClosedCallback(WaylandConnectionClosed callback);
    void reloadXkbOption();

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/wayland.conf");
    }
    void reloadConfig() override;

    void selfDiagnose();

private:
    void onConnectionCreated(WaylandConnection &conn);
    void onConnectionClosed(WaylandConnection &conn);
    void reloadXkbOptionReal();
    void setLayoutToGNOME();
    void setLayoutToKDE5();

    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(xcb, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(notifications, instance_->addonManager());

    Instance *instance_;
    WaylandConfig config_;
    bool isWaylandSession_ = false;
    std::unordered_map<std::string, WaylandConnection> conns_;
    HandlerTable<WaylandConnectionCreated> createdCallbacks_;
    HandlerTable<WaylandConnectionClosed> closedCallbacks_;
    FCITX_ADDON_EXPORT_FUNCTION(WaylandModule, addConnectionCreatedCallback);
    FCITX_ADDON_EXPORT_FUNCTION(WaylandModule, addConnectionClosedCallback);
    FCITX_ADDON_EXPORT_FUNCTION(WaylandModule, reloadXkbOption);
    FCITX_ADDON_EXPORT_FUNCTION(WaylandModule, openConnection);
    FCITX_ADDON_EXPORT_FUNCTION(WaylandModule, openConnectionSocket);

    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;
    std::unique_ptr<EventSourceTime> delayedReloadXkbOption_;
    std::unique_ptr<EventSourceTime> deferredDiagnose_;
};

FCITX_DECLARE_LOG_CATEGORY(wayland_log);

} // namespace fcitx

#endif // _FCITX_MODULES_WAYLAND_WAYLANDMODULE_H_
