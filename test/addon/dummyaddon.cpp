/*
 * Copyright (C) 2016~2016 by CSSlayer
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

#include "dummyaddon_public.h"
#include "fcitx-utils/metastring.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"

class DummyAddon : public fcitx::AddonInstance {
public:
    int addOne(int a) { return a + 1; }

    FCITX_ADDON_EXPORT_FUNCTION(DummyAddon, addOne);
};

class DummyAddonFactory : public fcitx::AddonFactory {
    virtual fcitx::AddonInstance *create(fcitx::AddonManager *) override {
        return new DummyAddon;
    }
};

FCITX_ADDON_FACTORY(DummyAddonFactory)
