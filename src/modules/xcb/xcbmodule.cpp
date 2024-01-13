/*
 * SPDX-FileCopyrightText: 2015-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "xcbmodule.h"

#include <utility>
#include "fcitx/instance.h"
#include "fcitx/misc_p.h"
#include "xcbconnection.h"

namespace fcitx {

FCITX_DEFINE_LOG_CATEGORY(xcb_log, "xcb");

XCBModule::XCBModule(Instance *instance) : instance_(instance) {
    reloadConfig();

    if (!containerContains(instance->addonManager().addonOptions("xcb"),
                           "nodefault")) {
        openConnection("");
    }
}

void XCBModule::reloadConfig() { readAsIni(config_, "conf/xcb.conf"); }

void XCBModule::openConnection(const std::string &name_) {
    openConnectionChecked(name_);
}

bool XCBModule::openConnectionChecked(const std::string &name_) {
    std::string name = name_;
    if (name.empty()) {
        auto *env = getenv("DISPLAY");
        if (env) {
            name = env;
            mainDisplay_ = name;
        }
    }
    if (name.empty() || conns_.count(name)) {
        return false;
    }

    XCBConnection *connection = nullptr;
    try {
        auto iter = conns_.emplace(std::piecewise_construct,
                                   std::forward_as_tuple(name),
                                   std::forward_as_tuple(this, name));
        connection = &iter.first->second;
    } catch (const std::exception &e) {
    }
    if (connection) {
        onConnectionCreated(*connection);
    }
    return connection != nullptr;
}

void XCBModule::removeConnection(const std::string &name) {
    // name might be a reference to the actual XCBConnection member, make a copy
    // to avoid read invalid value.
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return;
    }
    // Make a copy of name.
    std::string localName = name;
    onConnectionClosed(iter->second);
    conns_.erase(iter);
    FCITX_INFO() << "Disconnected from X11 Display " << localName;
    if (localName == mainDisplay_) {
        mainDisplay_.clear();
        char *sessionType = getenv("XDG_SESSION_TYPE");
        // We assume that empty XDG_SESSION_TYPE is X11.
        if ((isSessionType("x11") || !sessionType || sessionType[0] == '\0') &&
            instance_->exitWhenMainDisplayDisconnected()) {
            instance_->exit();
        }
    }
}

std::unique_ptr<HandlerTableEntry<XCBEventFilter>>
XCBModule::addEventFilter(const std::string &name, XCBEventFilter filter) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return nullptr;
    }
    return iter->second.addEventFilter(std::move(filter));
}

std::unique_ptr<HandlerTableEntry<XCBConnectionCreated>>
XCBModule::addConnectionCreatedCallback(XCBConnectionCreated callback) {
    auto result = createdCallbacks_.add(std::move(callback));

    for (auto &p : conns_) {
        auto &conn = p.second;
        (**result->handler())(conn.name(), conn.connection(), conn.screen(),
                              conn.focusGroup());
    }
    return result;
}

std::unique_ptr<HandlerTableEntry<XCBConnectionClosed>>
XCBModule::addConnectionClosedCallback(XCBConnectionClosed callback) {
    return closedCallbacks_.add(std::move(callback));
}

xkb_state *XCBModule::xkbState(const std::string &name) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return nullptr;
    }
    return iter->second.xkbState();
}

XkbRulesNames XCBModule::xkbRulesNames(const std::string &name) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return {};
    }
    return iter->second.xkbRulesNames();
}

std::unique_ptr<HandlerTableEntry<XCBSelectionNotifyCallback>>
XCBModule::addSelection(const std::string &name, const std::string &atom,
                        XCBSelectionNotifyCallback callback) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return nullptr;
    }
    return iter->second.addSelection(atom, std::move(callback));
}

std::unique_ptr<HandlerTableEntryBase>
XCBModule::convertSelection(const std::string &name, const std::string &atom,
                            const std::string &type,
                            XCBConvertSelectionCallback callback) {

    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return nullptr;
    }
    return iter->second.convertSelection(atom, type, std::move(callback));
}

void XCBModule::onConnectionCreated(XCBConnection &conn) {
    for (auto &callback : createdCallbacks_.view()) {
        callback(conn.name(), conn.connection(), conn.screen(),
                 conn.focusGroup());
    }
}

void XCBModule::onConnectionClosed(XCBConnection &conn) {
    for (auto &callback : closedCallbacks_.view()) {
        callback(conn.name(), conn.connection());
    }
}

xcb_atom_t XCBModule::atom(const std::string &name, const std::string &atom,
                           bool exists) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return XCB_ATOM_NONE;
    }
    return iter->second.atom(atom, exists);
}

xcb_ewmh_connection_t *XCBModule::ewmh(const std::string &name) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return nullptr;
    }
    return iter->second.ewmh();
}

bool XCBModule::isXWayland(const std::string &name) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return false;
    }
    return iter->second.isXWayland();
}

void XCBModule::setXkbOption(const std::string &name,
                             const std::string &option) {
    auto iter = conns_.find(name);
    if (iter == conns_.end()) {
        return;
    }
    iter->second.setXkbOption(option);
}

class XCBModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new XCBModule(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::XCBModuleFactory);
