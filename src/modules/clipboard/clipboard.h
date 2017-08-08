/*
 * Copyright (C) 2017~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#ifndef _FCITX_MODULES_CLIPBOARD_CLIPBOARD_H_
#define _FCITX_MODULES_CLIPBOARD_CLIPBOARD_H_

#include "fcitx-config/configuration.h"
#include "fcitx-config/enum.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/instance.h"
#include "xcb_public.h"
#include <map>

namespace fcitx {

FCITX_CONFIGURATION(ClipboardConfig,
                    Option<KeyList> triggerKey{this,
                                               "TriggerKey",
                                               "Trigger Key",
                                               {Key("Control+semicolon")}};
                    Option<int, IntConstrain> numOfEntries{
                        this, "Number of entries", "Number of entries", 5,
                        IntConstrain(3, 10)};);

class ClipboardState;
class Clipboard : public AddonInstance {
public:
    Clipboard(Instance *instance);
    ~Clipboard();

    Instance *instance() { return instance_; }

    void trigger(InputContext *inputContext);
    void updateUI(InputContext *inputContext);
    auto &factory() { return factory_; }

    void reloadConfig() override;

private:
    void primaryChanged(const std::string &name);
    void clipboardChanged(const std::string &name);

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
    std::list<std::string> history_;
    std::string primary_;
};
}

#endif // _FCITX_MODULES_CLIPBOARD_CLIPBOARD_H_
