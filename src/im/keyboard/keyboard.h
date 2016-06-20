/*
 * Copyright (C) 2016~2016 by CSSlayer
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
#ifndef _KEYBOARD_KEYBOARD_H_
#define _KEYBOARD_KEYBOARD_H_

#include "fcitx/inputmethodengine.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "isocodes.h"
#include "xkbrules.h"
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

namespace fcitx {

class Instance;

class KeyboardEngine : public InputMethodEngine {
public:
    KeyboardEngine(Instance *instance);
    ~KeyboardEngine();
    Instance *instance() { return m_instance; }
    void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) override;
    std::vector<InputMethodEntry> listInputMethods() override;

    uint32_t processCompose(uint32_t keyval, uint32_t state);

private:
    Instance *m_instance;
    IsoCodes m_isoCodes;
    XkbRules m_xkbRules;
    std::unique_ptr<struct xkb_context, decltype(&xkb_context_unref)> m_xkbContext;
    std::unique_ptr<struct xkb_compose_table, decltype(&xkb_compose_table_unref)> m_xkbComposeTable;
    std::unique_ptr<struct xkb_compose_state, decltype(&xkb_compose_state_unref)> m_xkbComposeState;
};

class KeyboardEngineFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override { return new KeyboardEngine(manager->instance()); }
};
}

#endif // _KEYBOARD_KEYBOARD_H_
