/*
 * Copyright (C) 2015~2015 by CSSlayer
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

#include "option.h"
#include "configuration.h"

namespace fcitx {
OptionBase::OptionBase(Configuration *parent, std::string path,
                       std::string description)
    : parent_(parent), path_(path), description_(description) {

    // Force the rule of "/" not allowed in option, so our live of GUI would be
    // easier.
    if (path.find('/') != std::string::npos) {
        throw std::invalid_argument(
            "/ is not allowed in option, option path is " + path);
    }
    parent_->addOption(this);
}

OptionBase::~OptionBase() {}

bool OptionBase::isDefault() const { return false; }

const std::string &OptionBase::path() const { return path_; }

const std::string &OptionBase::description() const { return description_; }

void OptionBase::dumpDescription(RawConfig &config) const {
    config.setValueByPath("Type", typeString());
    config.setValueByPath("Description", description_);
}
}
