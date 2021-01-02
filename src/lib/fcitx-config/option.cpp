/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "option.h"
#include <stdexcept>
#include "configuration.h"

namespace fcitx {

OptionBase::OptionBase(Configuration *parent, std::string path,
                       std::string description)
    : parent_(parent), path_(std::move(path)),
      description_(std::move(description)) {

    // Force the rule of "/" not allowed in option, so our live of GUI would be
    // easier.
    if (path_.find('/') != std::string::npos) {
        throw std::invalid_argument(
            "/ is not allowed in option, option path is " + path_);
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

ExternalOption::ExternalOption(Configuration *parent, std::string path,
                               std::string description, std::string uri)
    : OptionBase(parent, std::move(path), std::move(description)),
      externalUri_(std::move(uri)) {}

std::string ExternalOption::typeString() const { return "External"; }

void ExternalOption::reset() {}
bool ExternalOption::isDefault() const { return false; }

void ExternalOption::marshall(RawConfig &) const {}
bool ExternalOption::unmarshall(const RawConfig &, bool) { return true; }
std::unique_ptr<Configuration> ExternalOption::subConfigSkeleton() const {
    return nullptr;
}

bool ExternalOption::equalTo(const OptionBase &) const { return true; }
void ExternalOption::copyFrom(const OptionBase &) {}

bool ExternalOption::skipDescription() const { return false; }
bool ExternalOption::skipSave() const { return true; }
void ExternalOption::dumpDescription(RawConfig &config) const {
    OptionBase::dumpDescription(config);
    config.setValueByPath("External", externalUri_);
    // This field is required by dbus.
    config.setValueByPath("DefaultValue", "");
}

void SubConfigOption::dumpDescription(RawConfig &config) const {
    ExternalOption::dumpDescription(config);
    config.setValueByPath("LaunchSubConfig", "True");
}
} // namespace fcitx
