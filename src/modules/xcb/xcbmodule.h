/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_XCB_XCBMODULE_H_
#define _FCITX_MODULES_XCB_XCBMODULE_H_

#include <list>
#include <unordered_map>
#include <vector>
#include "fcitx-config/iniparser.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/handlertable.h"
#include "fcitx-utils/i18n.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/instance.h"
#include "xcb_public.h"
#include "xcbconnection.h"

namespace fcitx {

FCITX_CONFIGURATION(XCBConfig,
                    Option<bool> allowOverrideXKB{
                        this, "Allow Overriding System XKB Settings",
                        _("Allow Overriding System XKB Settings"), true};);

class XCBConnection;

class XCBModule final : public AddonInstance {
public:
    XCBModule(Instance *instance);

    void openConnection(const std::string &name);
    void removeConnection(std::string name);
    const XCBConfig &config() const { return config_; }
    Instance *instance() { return instance_; }

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/xcb.conf");
    }
    void reloadConfig() override;

    std::unique_ptr<HandlerTableEntry<XCBEventFilter>>
    addEventFilter(const std::string &name, XCBEventFilter filter);
    std::unique_ptr<HandlerTableEntry<XCBConnectionCreated>>
    addConnectionCreatedCallback(XCBConnectionCreated callback);
    std::unique_ptr<HandlerTableEntry<XCBConnectionClosed>>
    addConnectionClosedCallback(XCBConnectionClosed callback);
    struct xkb_state *xkbState(const std::string &name);
    XkbRulesNames xkbRulesNames(const std::string &name);
    std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>
    addSelection(const std::string &name, const std::string &atom,
                 XCBSelectionNotifyCallback callback);
    std::unique_ptr<HandlerTableEntryBase>
    convertSelection(const std::string &name, const std::string &atom,
                     const std::string &type,
                     XCBConvertSelectionCallback callback);

    xcb_atom_t atom(const std::string &name, const std::string &atom,
                    bool exists);
    xcb_ewmh_connection_t *ewmh(const std::string &name);

private:
    void onConnectionCreated(XCBConnection &conn);
    void onConnectionClosed(XCBConnection &conn);

    Instance *instance_;
    XCBConfig config_;
    std::unordered_map<std::string, XCBConnection> conns_;
    HandlerTable<XCBConnectionCreated> createdCallbacks_;
    HandlerTable<XCBConnectionClosed> closedCallbacks_;
    std::string mainDisplay_;
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, openConnection);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, addEventFilter);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, addConnectionCreatedCallback);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, addConnectionClosedCallback);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, xkbState);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, xkbRulesNames);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, addSelection);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, convertSelection);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, atom);
    FCITX_ADDON_EXPORT_FUNCTION(XCBModule, ewmh);
};

FCITX_DECLARE_LOG_CATEGORY(xcb_log);

#define FCITX_XCB_DEBUG() FCITX_LOGC(::fcitx::xcb_log, Debug)
#define FCITX_XCB_WARN() FCITX_LOGC(::fcitx::xcb_log, Debug)

} // namespace fcitx

#endif // _FCITX_MODULES_XCB_XCBMODULE_H_
