/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _FCITX_MODULES_UNICODE_UNICODE_H_
#define _FCITX_MODULES_UNICODE_UNICODE_H_

#include "charselectdata.h"
#include "fcitx-config/configuration.h"
#include "fcitx-config/enum.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/standardpath.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/instance.h"
#include <map>

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
}

#endif // _FCITX_MODULES_UNICODE_UNICODE_H_
