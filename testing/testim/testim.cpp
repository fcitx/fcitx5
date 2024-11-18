/*
 * SPDX-FileCopyrightText: 2020-2020 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "testim.h"
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>

namespace fcitx {

TestIM::TestIM(Instance *instance) : instance_(instance) {}

TestIM::~TestIM() {}

void TestIM::keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) {
    FCITX_INFO() << "IM " << entry.uniqueName()
                 << " received event: " << keyEvent.key().toString()
                 << keyEvent.isRelease();
    if (handler_) {
        handler_(entry, keyEvent);
    }
}

class TestIMFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new TestIM(manager->instance());
    }
};

} // namespace fcitx

FCITX_ADDON_FACTORY_V2(testim, fcitx::TestIMFactory);
