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

#include <cassert>
#include <map>
#include <exception>
#include <list>
#include <iostream>
#include <memory>

#include "configuration.h"

namespace fcitx {
struct ConfigurationPrivate {
    std::list<std::string> m_optionsOrder;
    std::map<std::string, OptionBase *> m_options;
};

Configuration::Configuration()
    : d_ptr(std::make_unique<ConfigurationPrivate>()) {}

Configuration::~Configuration() {}

void Configuration::dumpDescription(RawConfig &config) const {
    FCITX_D();
    std::shared_ptr<RawConfig> subRoot = config.get(typeName(), true);
    std::vector<std::unique_ptr<Configuration>> subConfigs;
    for (const auto &path : d->m_optionsOrder) {
        auto optionIter = d->m_options.find(path);
        assert(optionIter != d->m_options.end());
        auto option = optionIter->second;
        auto descConfigPtr = subRoot->get(option->path(), true);
        option->dumpDescription(*descConfigPtr);

        Configuration *subConfig = (option->subConfigSkeleton());

        if (subConfig) {
            subConfigs.emplace_back(subConfig);
        }
    }

    for (const auto &subConfigPtr : subConfigs) {
        subConfigPtr->dumpDescription(config);
    }
}

bool Configuration::compareHelper(const Configuration &other) const {
    FCITX_D();
    for (const auto &path : d->m_optionsOrder) {
        auto optionIter = d->m_options.find(path);
        assert(optionIter != d->m_options.end());
        auto otherOptionIter = other.d_func()->m_options.find(path);
        if (*optionIter->second != *otherOptionIter->second) {
            return false;
        }
    }
    return true;
}

void Configuration::copyHelper(const Configuration &other) {
    FCITX_D();
    for (const auto &path : d->m_optionsOrder) {
        auto optionIter = d->m_options.find(path);
        assert(optionIter != d->m_options.end());
        auto otherOptionIter = other.d_func()->m_options.find(path);
        assert(otherOptionIter != d->m_options.end());
        optionIter->second->copyFrom(*otherOptionIter->second);
    }
}

void Configuration::load(const RawConfig &config) {
    FCITX_D();
    for (const auto &path : d->m_optionsOrder) {
        auto subConfigPtr = config.get(path);
        auto option = d->m_options[path];
        if (!subConfigPtr) {
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
    for (const auto &path : d->m_optionsOrder) {
        auto subConfigPtr = config.get(path, true);
        auto iter = d->m_options.find(path);
        assert(iter != d->m_options.end());
        iter->second->marshall(*subConfigPtr);
        subConfigPtr->setComment(iter->second->description());
    }
}

void Configuration::addOption(OptionBase *option) {
    FCITX_D();
    if (d->m_options.count(option->path())) {
        throw std::logic_error("Duplicate option path");
    }

    d->m_optionsOrder.push_back(option->path());
    d->m_options[option->path()] = option;
}
}
