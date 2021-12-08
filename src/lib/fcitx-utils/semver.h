/*
 * SPDX-FileCopyrightText: 2021~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UTILS_SEMVER_H_
#define _FCITX_UTILS_SEMVER_H_

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "fcitxutils_export.h"
#include "log.h"
#include "macros.h"

namespace fcitx {

class FCITXUTILS_EXPORT PreReleaseId {
public:
    explicit PreReleaseId(uint32_t id);
    explicit PreReleaseId(std::string id);

    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(PreReleaseId);

    std::string toString() const;

    bool isNumeric() const;
    uint32_t numericId() const;
    const std::string &id() const;

    int compare(const PreReleaseId &other) const;

private:
    std::variant<std::string, uint32_t> value_;
};

/**
 * @brief Provide a Semantic version 2.0 implementation
 *
 * @since 5.0.6
 */
class FCITXUTILS_EXPORT SemanticVersion {
public:
    SemanticVersion() = default;
    FCITX_INLINE_DEFINE_DEFAULT_DTOR_COPY_AND_MOVE(SemanticVersion);

    static std::optional<SemanticVersion> parse(std::string_view data);

    std::string toString() const;

    FCITX_DECLARE_PROPERTY(uint32_t, (major), setMajor);
    FCITX_DECLARE_PROPERTY(uint32_t, (minor), setMinor);
    FCITX_DECLARE_PROPERTY(uint32_t, patch, setPatch);
    FCITX_DECLARE_PROPERTY(std::vector<PreReleaseId>, preReleaseIds,
                           setPreReleaseIds);
    FCITX_DECLARE_PROPERTY(std::vector<std::string>, buildIds, setBuildIds);
    bool isPreRelease() const;

    int compare(const SemanticVersion &version) const noexcept;

private:
    uint32_t major_ = 0;
    uint32_t minor_ = 1;
    uint32_t patch_ = 0;
    std::vector<PreReleaseId> preReleaseIds_;
    std::vector<std::string> buildIds_;
};

inline bool operator<(const PreReleaseId &lhs,
                      const PreReleaseId &rhs) noexcept {
    return lhs.compare(rhs) < 0;
}

inline bool operator>(const PreReleaseId &lhs,
                      const PreReleaseId &rhs) noexcept {
    return rhs < lhs;
}

inline bool operator==(const PreReleaseId &lhs,
                       const PreReleaseId &rhs) noexcept {
    return lhs.compare(rhs) == 0;
}

inline bool operator!=(const PreReleaseId &lhs,
                       const PreReleaseId &rhs) noexcept {
    return !(lhs == rhs);
}

inline bool operator<=(const PreReleaseId &lhs,
                       const PreReleaseId &rhs) noexcept {
    return !(lhs > rhs);
}

inline bool operator>=(const PreReleaseId &lhs,
                       const PreReleaseId &rhs) noexcept {
    return !(lhs < rhs);
}

inline bool operator<(const SemanticVersion &lhs,
                      const SemanticVersion &rhs) noexcept {
    return lhs.compare(rhs) < 0;
}

inline bool operator>(const SemanticVersion &lhs,
                      const SemanticVersion &rhs) noexcept {
    return rhs < lhs;
}

inline bool operator==(const SemanticVersion &lhs,
                       const SemanticVersion &rhs) noexcept {
    return lhs.compare(rhs) == 0;
}

inline bool operator!=(const SemanticVersion &lhs,
                       const SemanticVersion &rhs) noexcept {
    return !(lhs == rhs);
}

inline bool operator<=(const SemanticVersion &lhs,
                       const SemanticVersion &rhs) noexcept {
    return !(lhs > rhs);
}

inline bool operator>=(const SemanticVersion &lhs,
                       const SemanticVersion &rhs) noexcept {
    return !(lhs < rhs);
}

inline LogMessageBuilder &operator<<(LogMessageBuilder &builder,
                                     const SemanticVersion &version) {
    builder << "SemanticVersion(" << version.toString() << ")";
    return builder;
}

} // namespace fcitx

#endif // _FCITX_UTILS_SEMVER_H_
