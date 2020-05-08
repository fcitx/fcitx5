/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fcitx-utils/metastring.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "dummyaddon_public.h"

class DummyAddon : public fcitx::AddonInstance {
public:
    int addOne(int a) { return a + 1; }

    FCITX_ADDON_EXPORT_FUNCTION(DummyAddon, addOne);
};

class DummyAddonFactory : public fcitx::AddonFactory {
    fcitx::AddonInstance *create(fcitx::AddonManager *) override {
        return new DummyAddon;
    }
};

FCITX_ADDON_FACTORY(DummyAddonFactory)
