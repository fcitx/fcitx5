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
    AddonConfigBase, Option<I18NString> name{this, "Name", "Addon Name"};
    Option<I18NString> comment{this, "Comment", "Comment"};
    Option<SemanticVersion> version{this, "Version", "Addon Version"};
    Option<std::string> type{this, "Type", "Addon Type"};
    Option<std::string> library{this, "Library", "Addon Library"};
    Option<bool> configurable{this, "Configurable", "Configurable", false};
    Option<bool> enabled{this, "Enabled", "Enabled", true};
    Option<AddonCategory> category{this, "Category", "Category"};
    Option<std::vector<std::string>> dependencies{this, "Dependencies",
                                                  "Dependencies"};
    Option<std::vector<std::string>> optionalDependencies{
        this, "OptionalDependencies", "Optional Dependencies"};
    Option<bool> onDemand{this, "OnDemand", "Load only on request", false};
    Option<int> uiPriority{this, "UIPriority", "User interface priority", 0};)

FCITX_CONFIGURATION(AddonConfig,
                    Option<AddonConfigBase> addon{this, "Addon", "Addon"};)

namespace {

void parseDependencies(const std::vector<std::string> &data,
                       std::vector<std::string> &dependencies,
                       std::vector<std::tuple<std::string, SemanticVersion>>
                           &dependenciesWithVersion) {
    dependencies.clear();
    dependenciesWithVersion.clear();
    for (const auto &item : data) {
        auto tokens = stringutils::split(item, ":");
        if (tokens.size() == 1) {
            dependencies.push_back(tokens[0]);
            dependenciesWithVersion.emplace_back(tokens[0], SemanticVersion{});
        } else if (tokens.size() == 2) {
            auto version = SemanticVersion::parse(tokens[1]);
            if (version) {
                dependencies.push_back(tokens[0]);
                dependenciesWithVersion.emplace_back(tokens[0],
                                                     version.value());
            }
        }
    }
}
} // namespace

class AddonInfoPrivate : public AddonConfig {
public:
    AddonInfoPrivate(const std::string &name) : uniqueName_(name) {}

    bool valid_ = false;
    std::string uniqueName_;
    OverrideEnabled overrideEnabled_ = OverrideEnabled::NotSet;
    std::vector<std::string> dependencies_;
    std::vector<std::string> optionalDependencies_;
    std::vector<std::tuple<std::string, SemanticVersion>>
        dependenciesWithVersion_;
    std::vector<std::tuple<std::string, SemanticVersion>>
        optionalDependenciesWithVersion_;
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
    return d->dependencies_;
}

const std::vector<std::string> &AddonInfo::optionalDependencies() const {
    FCITX_D();
    return d->optionalDependencies_;
}

const std::vector<std::tuple<std::string, SemanticVersion>> &
AddonInfo::dependenciesWithVersion() const {
    FCITX_D();
    return d->dependenciesWithVersion_;
}

const std::vector<std::tuple<std::string, SemanticVersion>> &
AddonInfo::optionalDependenciesWithVersion() const {
    FCITX_D();
    return d->optionalDependenciesWithVersion_;
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

    parseDependencies(*d->addon->dependencies, d->dependencies_,
                      d->dependenciesWithVersion_);
    parseDependencies(*d->addon->optionalDependencies, d->optionalDependencies_,
                      d->optionalDependenciesWithVersion_);

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

const SemanticVersion &AddonInfo::version() const {
    FCITX_D();
    return *d->addon->version;
}

void AddonInfo::setOverrideEnabled(OverrideEnabled overrideEnabled) {
    FCITX_D();
    d->overrideEnabled_ = overrideEnabled;
}
} // namespace fcitx
