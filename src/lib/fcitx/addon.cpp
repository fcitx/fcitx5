/*
 * Copyright (C) 2015~2015 by CSSlayer
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

#include "addon.h"
#include "fcitx-config/configuration.h"
namespace fcitx
{

FCITX_CONFIGURATION(AddonConfig,
    fcitx::Option<std::string> name{this, "Addon/Name", "Addon Name"};
    fcitx::Option<fcitx::AddonType> type{this, "Addon/Type", "Addon Type"};
)


struct AddonInfoPrivate : public AddonConfig
{
    bool valid = false;
};

AddonInfo::AddonInfo() : d_ptr(std::make_unique<AddonInfoPrivate>())
{

}

AddonInfo::~AddonInfo()
{

}

bool AddonInfo::isValid() const
{
    FCITX_D();
    return d->valid;
}

const std::string& AddonInfo::name() const
{
    FCITX_D();
    return d->name.value();

}

AddonType AddonInfo::type() const
{
    FCITX_D();
    return d->type.value();
}

void AddonInfo::loadInfo(RawConfig& config)
{
    FCITX_D();
    d->load(config);

    // Validate more information
    d->valid = true;
}


}
