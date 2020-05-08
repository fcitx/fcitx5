/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "addoninfo.h"
#include "fcitx-config/configuration.h"
namespace fcitx {

FCITX_CONFIGURATION(
    AddonConfigBase, fcitx::Option<I18NString> name{this, "Name", "Addon Name"};
    fcitx::Option<I18NString> comment{this, "Comment", "Comment"};
    fcitx::Option<std::string> type{this, "Type", "Addon Type"};
    fcitx::Option<std::string> library{this, "Library", "Addon Library"};
    fcitx::Option<bool> configurable{this, "Configurable", "Configurable",
                                     false};
    fcitx::Option<bool> enabled{this, "Enabled", "Enabled", true};
    fcitx::Option<AddonCategory> category{this, "Category", "Category"};
    fcitx::Option<std::vector<std::string>> dependencies{this, "Dependencies",
                                                         "Dependencies"};
    fcitx::Option<std::vector<std::string>> optionalDependencies{
        this, "OptionalDependencies", "Optional Dependencies"};
    fcitx::Option<bool> onDemand{this, "OnDemand", "Load only on request",
                                 false};
    fcitx::Option<int> uiPriority{this, "UIPriority", "User interface priority",
                                  0};)

FCITX_CONFIGURATION(AddonConfig,
                    Option<AddonConfigBase> addon{this, "Addon", "Addon"};)

class AddonInfoPrivate : public AddonConfig {
public:
    AddonInfoPrivate(const std::string &name) : uniqueName_(name) {}

    bool valid_ = false;
    std::string uniqueName_;
    OverrideEnabled overrideEnabled_ = OverrideEnabled::NotSet;
};

AddonInfo::AddonInfo(const std::string &name)
    : d_ptr(std::make_unique<AddonInfoPrivate>(name)) {}

AddonInfo::~AddonInfo() {}

bool AddonInfo::isValid() const {
    FCITX_D();
    return d->valid_;
}

const std::string &AddonInfo::uniqueName() const {
    FCITX_D();
    return d->uniqueName_;
}

const I18NString &AddonInfo::name() const {
    FCITX_D();
    return d->addon->name.value();
}

const I18NString &AddonInfo::comment() const {
    FCITX_D();
    return d->addon->comment.value();
}

const std::string &AddonInfo::type() const {
    FCITX_D();
    return d->addon->type.value();
}

AddonCategory AddonInfo::category() const {
    FCITX_D();
    return d->addon->category.value();
}

const std::string &AddonInfo::library() const {
    FCITX_D();
    return d->addon->library.value();
}

const std::vector<std::string> &AddonInfo::dependencies() const {
    FCITX_D();
    return d->addon->dependencies.value();
}

const std::vector<std::string> &AddonInfo::optionalDependencies() const {
    FCITX_D();
    return d->addon->optionalDependencies.value();
}

bool AddonInfo::onDemand() const {
    FCITX_D();
    return d->addon->onDemand.value();
}

int AddonInfo::uiPriority() const {
    FCITX_D();
    return d->addon->uiPriority.value();
}

void AddonInfo::load(const RawConfig &config) {
    FCITX_D();
    d->load(config);

    // Validate more information
    d->valid_ = !(d->uniqueName_.empty()) &&
                !(d->addon->type.value().empty()) &&
                !(d->addon->library.value().empty());
}

bool AddonInfo::isEnabled() const {
    FCITX_D();
    if (d->overrideEnabled_ == OverrideEnabled::NotSet) {
        return *d->addon->enabled;
    }
    return d->overrideEnabled_ == OverrideEnabled::Enabled;
}

bool AddonInfo::isDefaultEnabled() const {
    FCITX_D();
    return *d->addon->enabled;
}

bool AddonInfo::isConfigurable() const {
    FCITX_D();
    return *d->addon->configurable;
}

void AddonInfo::setOverrideEnabled(OverrideEnabled overrideEnabled) {
    FCITX_D();
    d->overrideEnabled_ = overrideEnabled;
}
} // namespace fcitx
