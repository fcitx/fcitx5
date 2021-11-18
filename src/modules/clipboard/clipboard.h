/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_CLIPBOARD_CLIPBOARD_H_
#define _FCITX_MODULES_CLIPBOARD_CLIPBOARD_H_

#include <map>
#include "fcitx-config/configuration.h"
#include "fcitx-config/enum.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/instance.h"
#include "clipboard_public.h"

#ifdef ENABLE_X11
#include "xcb_public.h"
#endif
#ifdef WAYLAND_FOUND
#include "wayland_public.h"
#include "waylandclipboard.h"
#include "zwlr_data_control_manager_v1.h"
#endif

namespace fcitx {

constexpr size_t MAX_CLIPBOARD_SIZE = 4096;

FCITX_CONFIGURATION(
    ClipboardConfig, KeyListOption triggerKey{this,
                                              "TriggerKey",
                                              _("Trigger Key"),
                                              {Key("Control+semicolon")},
                                              KeyListConstrain()};
    KeyListOption pastePrimaryKey{
        this, "PastePrimaryKey", _("Paste Primary"), {}, KeyListConstrain()};
    Option<int, IntConstrain> numOfEntries{this, "Number of entries",
                                           _("Number of entries"), 5,
                                           IntConstrain(3, 30)};);

class ClipboardState;
class Clipboard final : public AddonInstance {
    static constexpr char configFile[] = "conf/clipboard.conf";

public:
    Clipboard(Instance *instance);
    ~Clipboard();

    Instance *instance() { return instance_; }

    void trigger(InputContext *inputContext);
    void updateUI(InputContext *inputContext);
    auto &factory() { return factory_; }

    void reloadConfig() override { readAsIni(config_, configFile); }

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, configFile);
    }

    std::string primary(const InputContext *ic);
    std::string clipboard(const InputContext *ic);

    void setPrimary(const std::string &name, const std::string &str);
    void setClipboard(const std::string &name, const std::string &str);

private:
    void primaryChanged(const std::string &name);
    void clipboardChanged(const std::string &name);
    FCITX_ADDON_EXPORT_FUNCTION(Clipboard, primary);
    FCITX_ADDON_EXPORT_FUNCTION(Clipboard, clipboard);
#ifdef ENABLE_X11
    FCITX_ADDON_DEPENDENCY_LOADER(xcb, instance_->addonManager());
#endif
#ifdef WAYLAND_FOUND
    FCITX_ADDON_DEPENDENCY_LOADER(wayland, instance_->addonManager());
#endif

    Instance *instance_;
    std::vector<std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>>
        eventHandlers_;
    KeyList selectionKeys_;
    ClipboardConfig config_;
    FactoryFor<ClipboardState> factory_;

#ifdef ENABLE_X11
    std::unique_ptr<HandlerTableEntry<XCBConnectionCreated>>
        xcbCreatedCallback_;
    std::unique_ptr<HandlerTableEntry<XCBConnectionClosed>> xcbClosedCallback_;
    std::unordered_map<std::string,
                       std::vector<std::unique_ptr<HandlerTableEntryBase>>>
        selectionCallbacks_;

    std::unique_ptr<HandlerTableEntryBase> primaryCallback_;
    std::unique_ptr<HandlerTableEntryBase> clipboardCallback_;
#endif

#ifdef WAYLAND_FOUND
    std::unique_ptr<HandlerTableEntry<WaylandConnectionCreated>>
        waylandCreatedCallback_;
    std::unique_ptr<HandlerTableEntry<WaylandConnectionClosed>>
        waylandClosedCallback_;
    std::unordered_map<std::string, std::unique_ptr<WaylandClipboard>>
        waylandClipboards_;
#endif
    OrderedSet<std::string> history_;
    std::string primary_;
};

FCITX_DECLARE_LOG_CATEGORY(clipboard_log);

#define FCITX_CLIPBOARD_DEBUG() FCITX_LOGC(::fcitx::clipboard_log, Debug)
} // namespace fcitx

#endif // _FCITX_MODULES_CLIPBOARD_CLIPBOARD_H_
