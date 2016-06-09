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

#include "addoninfo.h"
#include "fcitx-config/configuration.h"
namespace fcitx {

FCITX_CONFIGURATION(AddonConfig, fcitx::Option<std::string> name{this, "Addon/Name", "Addon Name"};
                    fcitx::Option<std::string> type{this, "Addon/Type", "Addon Type"};
                    fcitx::Option<std::string> library{this, "Addon/Library", "Addon Library"};
                    fcitx::Option<bool> enabled{this, "Addon/Enabled", "Enabled", true};
                    fcitx::Option<AddonCategory> category{this, "Addon/Category", "Category"};
                    fcitx::Option<std::vector<std::string>> dependencies{this, "Addon/Dependencies", "Dependencies"};)

class AddonInfoPrivate : public AddonConfig {
public:
    bool valid = false;
};

AddonInfo::AddonInfo() : d_ptr(std::make_unique<AddonInfoPrivate>()) {}

AddonInfo::~AddonInfo() {}

bool AddonInfo::isValid() const {
    FCITX_D();
    return d->valid;
}

const std::string &AddonInfo::name() const {
    FCITX_D();
    return d->name.value();
}

const std::string &AddonInfo::type() const {
    FCITX_D();
    return d->type.value();
}

const std::string &AddonInfo::library() const {
    FCITX_D();
    return d->library.value();
}

const std::vector<std::string> &AddonInfo::dependencies() const {
    FCITX_D();
    return d->dependencies.value();
}

void AddonInfo::loadInfo(const RawConfig &config) {
    FCITX_D();
    d->load(config);

    // Validate more information
    d->valid =
        !(d->name.value().empty()) && !(d->type.value().empty()) && !(d->library.value().empty()) && d->enabled.value();
}
}
