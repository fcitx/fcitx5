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

#include <cassert>
#include <exception>
#include <list>
#include <memory>
#include <unordered_map>

#include "configuration.h"
#include "fcitx-utils/standardpath.h"

namespace fcitx {
class ConfigurationPrivate {
public:
    std::list<std::string> optionsOrder_;
    std::unordered_map<std::string, OptionBase *> options_;
};

Configuration::Configuration()
    : d_ptr(std::make_unique<ConfigurationPrivate>()) {}

Configuration::~Configuration() {}

void Configuration::dumpDescription(RawConfig &config) const {
    FCITX_D();
    std::shared_ptr<RawConfig> subRoot = config.get(typeName(), true);
    std::vector<std::unique_ptr<Configuration>> subConfigs;
    for (const auto &path : d->optionsOrder_) {
        auto optionIter = d->options_.find(path);
        assert(optionIter != d->options_.end());
        auto option = optionIter->second;
        auto descConfigPtr = subRoot->get(option->path(), true);
        if (option->skipDescription()) {
            continue;
        }
        option->dumpDescription(*descConfigPtr);

        auto subConfig = (option->subConfigSkeleton());

        if (subConfig) {
            subConfigs.emplace_back(std::move(subConfig));
        }
    }

    for (const auto &subConfigPtr : subConfigs) {
        subConfigPtr->dumpDescription(config);
    }
}

bool Configuration::compareHelper(const Configuration &other) const {
    FCITX_D();
    for (const auto &path : d->optionsOrder_) {
        auto optionIter = d->options_.find(path);
        assert(optionIter != d->options_.end());
        auto otherOptionIter = other.d_func()->options_.find(path);
        if (*optionIter->second != *otherOptionIter->second) {
            return false;
        }
    }
    return true;
}

void Configuration::copyHelper(const Configuration &other) {
    FCITX_D();
    for (const auto &path : d->optionsOrder_) {
        auto optionIter = d->options_.find(path);
        assert(optionIter != d->options_.end());
        auto otherOptionIter = other.d_func()->options_.find(path);
        assert(otherOptionIter != d->options_.end());
        optionIter->second->copyFrom(*otherOptionIter->second);
    }
}

void Configuration::load(const RawConfig &config, bool partial) {
    FCITX_D();
    for (const auto &path : d->optionsOrder_) {
        auto subConfigPtr = config.get(path);
        auto option = d->options_[path];
        if (!subConfigPtr && !partial) {
            option->reset();
            continue;
        }
        if (!option->unmarshall(*subConfigPtr)) {
            option->reset();
        }
    }
}

void Configuration::save(RawConfig &config) const {
    FCITX_D();
    for (const auto &path : d->optionsOrder_) {
        auto subConfigPtr = config.get(path, true);
        auto iter = d->options_.find(path);
        assert(iter != d->options_.end());
        iter->second->marshall(*subConfigPtr);
        subConfigPtr->setComment(iter->second->description());
    }
}

void Configuration::addOption(OptionBase *option) {
    FCITX_D();
    if (d->options_.count(option->path())) {
        throw std::logic_error("Duplicate option path");
    }

    d->optionsOrder_.push_back(option->path());
    d->options_[option->path()] = option;
}
}
