/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "configuration.h"
#include <cassert>
#include <exception>
#include <list>
#include <memory>
#include <stdexcept>
#include <unordered_map>
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
        auto *option = optionIter->second;
        if (option->skipDescription()) {
            continue;
        }
        auto descConfigPtr = subRoot->get(option->path(), true);
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
        auto *option = d->options_[path];
        if (!subConfigPtr) {
            if (!partial) {
                option->reset();
            }
            continue;
        }
        if (!option->unmarshall(*subConfigPtr, partial)) {
            option->reset();
        }
    }
}

void Configuration::save(RawConfig &config) const {
    FCITX_D();
    for (const auto &path : d->optionsOrder_) {
        auto iter = d->options_.find(path);
        assert(iter != d->options_.end());
        if (iter->second->skipSave()) {
            continue;
        }
        auto subConfigPtr = config.get(path, true);
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

void Configuration::syncDefaultValueToCurrent() {
    FCITX_D();
    for (const auto &path : d->optionsOrder_) {
        auto iter = d->options_.find(path);
        assert(iter != d->options_.end());
        if (auto optionV2 = dynamic_cast<OptionBaseV2 *>(iter->second)) {
            optionV2->syncDefaultValueToCurrent();
        }
    }
}

} // namespace fcitx
