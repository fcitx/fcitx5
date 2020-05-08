/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_MODULES_UNICODE_UNICODE_H_
#define _FCITX_MODULES_UNICODE_UNICODE_H_

#include <map>
#include "fcitx-config/configuration.h"
#include "fcitx-config/enum.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/instance.h"
#include "charselectdata.h"

namespace fcitx {

class UnicodeState;
class Unicode : public AddonInstance {
public:
    Unicode(Instance *instance);
    ~Unicode();

    Instance *instance() { return instance_; }

    void trigger(InputContext *inputContext);
    void updateUI(InputContext *inputContext);
    auto &factory() { return factory_; }

    const CharSelectData &data() const { return data_; }

private:
    Instance *instance_;
    Key toggleKey_;
    CharSelectData data_;
    std::vector<std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>>
        eventHandlers_;
    KeyList selectionKeys_;
    FactoryFor<UnicodeState> factory_;
};
} // namespace fcitx

#endif // _FCITX_MODULES_UNICODE_UNICODE_H_
