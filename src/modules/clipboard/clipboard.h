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
#include "fcitx-utils/key.h"
#include "fcitx-utils/misc_p.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/instance.h"
#include "clipboard_public.h"
#include "xcb_public.h"

namespace fcitx {

FCITX_CONFIGURATION(ClipboardConfig,
                    KeyListOption triggerKey{this,
                                             "TriggerKey",
                                             "Trigger Key",
                                             {Key("Control+semicolon")},
                                             KeyListConstrain()};
                    Option<int, IntConstrain> numOfEntries{
                        this, "Number of entries", "Number of entries", 5,
                        IntConstrain(3, 30)};);

class ClipboardState;
class Clipboard final : public AddonInstance {
public:
    Clipboard(Instance *instance);
    ~Clipboard();

    Instance *instance() { return instance_; }

    void trigger(InputContext *inputContext);
    void updateUI(InputContext *inputContext);
    auto &factory() { return factory_; }

    void reloadConfig() override;

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/clipboard.conf");
    }

    std::string primary(const InputContext *ic);
    std::string clipboard(const InputContext *ic);

private:
    void primaryChanged(const std::string &name);
    void clipboardChanged(const std::string &name);
    FCITX_ADDON_EXPORT_FUNCTION(Clipboard, primary);
    FCITX_ADDON_EXPORT_FUNCTION(Clipboard, clipboard);

    Instance *instance_;
    std::vector<std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>>
        eventHandlers_;
    KeyList selectionKeys_;
    ClipboardConfig config_;
    FactoryFor<ClipboardState> factory_;
    AddonInstance *xcb_;

    std::unique_ptr<HandlerTableEntry<XCBConnectionCreated>>
        xcbCreatedCallback_;
    std::unique_ptr<HandlerTableEntry<XCBConnectionClosed>> xcbClosedCallback_;
    std::unordered_map<std::string,
                       std::vector<std::unique_ptr<HandlerTableEntryBase>>>
        selectionCallbacks_;
    std::unique_ptr<HandlerTableEntryBase> primaryCallback_;
    std::unique_ptr<HandlerTableEntryBase> clipboardCallback_;
    OrderedSet<std::string> history_;
    std::string primary_;
};
} // namespace fcitx

#endif // _FCITX_MODULES_CLIPBOARD_CLIPBOARD_H_
