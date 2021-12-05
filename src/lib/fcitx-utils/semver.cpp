/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "semver.h"
#include <charconv>
#include <fmt/format.h>
#include "charutils.h"
#include "misc.h"
#include "stringutils.h"

namespace fcitx {

bool isIdChar(char c) {
    return charutils::islower(c) || charutils::isupper(c) || c == '-' ||
           charutils::isdigit(c) || c == '.';
}

std::optional<uint32_t> consumeNumericIdentifier(std::string_view &str) {
    auto endOfNum =
        std::find_if_not(str.begin(), str.end(), charutils::isdigit);
    auto length = std::distance(str.begin(), endOfNum);
    if (length == 0) {
        return std::nullopt;
    }
    if (str[0] == '0' && length != 1) {
        return std::nullopt;
    }

    auto numberStr = str.substr(0, length);
    uint32_t number;
    if (auto [p, ec] =
            std::from_chars(numberStr.begin(), numberStr.end(), number);
        ec == std::errc()) {
        str.remove_prefix(length);
        return number;
    }
    return std::nullopt;
}

std::optional<std::vector<PreReleaseId>>
consumePrereleaseIds(std::string_view &data) {
    std::vector<PreReleaseId> preReleaseIds;
    auto endOfVersion = std::find_if_not(data.begin(), data.end(), isIdChar);
    auto length = std::distance(data.begin(), endOfVersion);
    auto idString = data.substr(0, length);
    auto ids = stringutils::split(idString, ".",
                                  stringutils::SplitBehavior::KeepEmpty);
    for (const auto &id : ids) {
        if (id.empty()) {
            return std::nullopt;
        }
        // If it's numeric, it need to be a valid numeric.
        // Otherwise it can be anything.
        if (std::all_of(id.begin(), id.end(), charutils::isdigit)) {
            std::string_view idView(id);
            auto result = consumeNumericIdentifier(idView);
            if (result && idView.empty()) {
                preReleaseIds.emplace_back(result.value());
            } else {
                return std::nullopt;
            }
        } else {
            preReleaseIds.emplace_back(id);
        }
    }
    data.remove_prefix(length);
    return preReleaseIds;
}

std::optional<std::vector<std::string>> consumeBuild(std::string_view &data) {
    if (std::all_of(data.begin(), data.end(), isIdChar)) {
        auto ids = stringutils::split(data, ".",
                                      stringutils::SplitBehavior::KeepEmpty);
        if (std::any_of(ids.begin(), ids.end(),
                        [](const auto &id) { return id.empty(); })) {
            return std::nullopt;
        }
        data.remove_prefix(data.size());
        return ids;
    }
    return std::nullopt;
}

PreReleaseId::PreReleaseId(uint32_t id) : value_(id) {}

PreReleaseId::PreReleaseId(std::string id) : value_(std::move(id)) {}

std::string PreReleaseId::toString() const {
    if (isNumeric()) {
        return std::to_string(numericId());
    }
    return id();
}

int PreReleaseId::compare(const PreReleaseId &other) const {
    auto isNum = isNumeric();
    auto otherIsNum = other.isNumeric();
    if (isNum != otherIsNum) {
        // this is num and other is not num, return -1;
        return isNum ? -1 : 1;
    }
    if (isNum) {
        return numericId() - other.numericId();
    }

    return id().compare(other.id());
}

const std::string &PreReleaseId::id() const {
    return std::get<std::string>(value_);
}

uint32_t PreReleaseId::numericId() const { return std::get<uint32_t>(value_); }

bool PreReleaseId::isNumeric() const {
    return std::holds_alternative<uint32_t>(value_);
}

void SemanticVersion::setBuildIds(std::vector<std::string> build) {
    buildIds_ = std::move(build);
}

void SemanticVersion::setMajor(uint32_t major) { major_ = major; }

void SemanticVersion::setMinor(uint32_t minor) { minor_ = minor; }

void SemanticVersion::setPatch(uint32_t patch) { patch_ = patch; }

std::string SemanticVersion::toString() const {
    std::string result = fmt::format("{0}.{1}.{2}", major_, minor_, patch_);
    if (!preReleaseIds_.empty()) {
        result.append("-");
        result.append(preReleaseIds_.front().toString());
        for (const auto &item : MakeIterRange(std::next(preReleaseIds_.begin()),
                                              preReleaseIds_.end())) {
            result.append(".");
            result.append(item.toString());
        }
    }

    if (!buildIds_.empty()) {
        result.append("+");
        result.append(stringutils::join(buildIds_, "."));
    }

    return result;
}

void SemanticVersion::setPreReleaseIds(std::vector<PreReleaseId> ids) {
    preReleaseIds_ = std::move(ids);
}

uint32_t(SemanticVersion::major)() const { return major_; }

uint32_t(SemanticVersion::minor)() const { return minor_; }

uint32_t SemanticVersion::patch() const { return patch_; }

const std::vector<PreReleaseId> &SemanticVersion::preReleaseIds() const {
    return preReleaseIds_;
}

const std::vector<std::string> &SemanticVersion::buildIds() const {
    return buildIds_;
}

bool SemanticVersion::isPreRelease() const { return !preReleaseIds_.empty(); }

std::optional<SemanticVersion> SemanticVersion::parse(std::string_view data) {
    SemanticVersion version;
    if (auto result = consumeNumericIdentifier(data)) {
        version.setMajor(result.value());
    } else {
        return std::nullopt;
    }

    if (data.empty() || data.front() != '.') {
        return std::nullopt;
    }
    data.remove_prefix(1);

    if (auto result = consumeNumericIdentifier(data)) {
        version.setMinor(result.value());
    } else {
        return std::nullopt;
    }

    if (data.empty() || data.front() != '.') {
        return std::nullopt;
    }
    data.remove_prefix(1);

    if (auto result = consumeNumericIdentifier(data)) {
        version.setPatch(result.value());
    } else {
        return std::nullopt;
    }

    if (data.empty()) {
        return version;
    }

    if (data[0] == '-') {
        data.remove_prefix(1);
        if (auto result = consumePrereleaseIds(data)) {
            version.setPreReleaseIds(std::move(result.value()));
        } else {
            return std::nullopt;
        }
    }

    if (data.empty()) {
        return version;
    }

    if (data[0] == '+') {
        data.remove_prefix(1);
        if (auto result = consumeBuild(data)) {
            version.setBuildIds(std::move(result.value()));
        } else {
            return std::nullopt;
        }
    }

    if (!data.empty()) {
        return std::nullopt;
    }
    return version;
}

int SemanticVersion::compare(const SemanticVersion &other) const noexcept {
    if (major_ != other.major_) {
        return major_ - other.major_;
    }

    if (minor_ != other.minor_) {
        return minor_ - other.minor_;
    }

    if (patch_ != other.patch_) {
        return patch_ - other.patch_;
    }

    bool preRelease = isPreRelease();
    bool otherIsPrerelase = other.isPreRelease();

    if (preRelease != otherIsPrerelase) {
        return preRelease ? -1 : 1;
    }

    if (!preRelease) {
        return 0;
    }

    for (size_t i = 0, e = std::min(preReleaseIds_.size(),
                                    other.preReleaseIds_.size());
         i < e; i++) {
        auto result = preReleaseIds_[i].compare(other.preReleaseIds_[i]);
        if (result != 0) {
            return result;
        }
    }

    return static_cast<ssize_t>(preReleaseIds_.size()) -
           static_cast<ssize_t>(other.preReleaseIds_.size());
}

} // namespace fcitx
