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

#include "simpleui.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"

namespace fcitx {

SimpleUI::SimpleUI(Instance *instance) : instance_(instance) {}

SimpleUI::~SimpleUI() {}

void SimpleUI::resume() {}

void SimpleUI::suspend() {}

void SimpleUI::update(UserInterfaceComponent component) {
    switch (component) {
    case UserInterfaceComponent::StatusArea:
        printStatusArea();
        break;
    case UserInterfaceComponent::InputPanel:
        printInputPanel();
        break;
    }
}

void SimpleUI::printStatusArea() {}

void SimpleUI::printInputPanel() {}

class SimpleUIFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override { return new SimpleUI(manager->instance()); }
};
}

FCITX_ADDON_FACTORY(fcitx::SimpleUIFactory);
