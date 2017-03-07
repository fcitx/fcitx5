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
#ifndef _FCITX_IM_KEYBOARD_KEYBOARD_H_
#define _FCITX_IM_KEYBOARD_KEYBOARD_H_

#include "fcitx-config/configuration.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputmethodengine.h"
#include "fcitx/instance.h"
#include "isocodes.h"
#include "xkbrules.h"
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>

namespace fcitx {

class Instance;

FCITX_CONFIG_ENUM(ChooseModifier, None, Alt, Control, Super);

FCITX_CONFIGURATION(
    KeyboardEngineConfig,
    fcitx::Option<int, IntConstrain> pageSize{this, "PageSize", "Page size", 5, IntConstrain(3, 10)};
    fcitx::Option<ChooseModifier> chooseModifier{this, "Choose Modifier", "Choose key modifier", ChooseModifier::Alt};
    fcitx::Option<KeyList> hintTrigger{this, "Hint Trigger", "Trigger hint mode", {Key("Control+Alt+H")}};);

struct KeyboardEngineState;

class KeyboardEngine : public InputMethodEngine {
public:
    KeyboardEngine(Instance *instance);
    ~KeyboardEngine();
    Instance *instance() { return instance_; }
    void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) override;
    std::vector<InputMethodEntry> listInputMethods() override;
    void reloadConfig() override;
    void reset(const InputMethodEntry &entry, InputContextEvent &event) override;

    uint32_t processCompose(KeyboardEngineState *state, uint32_t keyval);

    AddonInstance *spell() {
        if (!spell_) {
            spell_ = instance_->addonManager().addon("spell", true);
        }
        return spell_;
    }

    AddonInstance *notifications() {
        if (!notifications_) {
            notifications_ = instance_->addonManager().addon("notifications", true);
        }
        return notifications_;
    }

    void updateCandidate(const InputMethodEntry &entry, InputContext *inputContext);

    xkb_compose_table *xkbComposeTable() const { return xkbComposeTable_.get(); }

private:
    void commitBuffer(InputContext *inputContext);

    Instance *instance_;
    AddonInstance *spell_ = nullptr;
    AddonInstance *notifications_ = nullptr;
    KeyboardEngineConfig config_;
    IsoCodes isoCodes_;
    XkbRules xkbRules_;
    std::string ruleName_;
    std::unique_ptr<struct xkb_context, decltype(&xkb_context_unref)> xkbContext_;
    std::unique_ptr<struct xkb_compose_table, decltype(&xkb_compose_table_unref)> xkbComposeTable_;
    KeyList selectionKeys_;
};

class KeyboardEngineFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override { return new KeyboardEngine(manager->instance()); }
};
}

#endif // _FCITX_IM_KEYBOARD_KEYBOARD_H_
