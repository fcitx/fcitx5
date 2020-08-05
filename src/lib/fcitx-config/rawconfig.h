/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CONFIG_RAWCONFIG_H_
#define _FCITX_CONFIG_RAWCONFIG_H_

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include "fcitxconfig_export.h"

namespace fcitx {

class RawConfig;

typedef std::shared_ptr<RawConfig> RawConfigPtr;

class RawConfigPrivate;
class FCITXCONFIG_EXPORT RawConfig {
public:
    RawConfig();
    FCITX_DECLARE_VIRTUAL_DTOR_COPY(RawConfig)

    std::shared_ptr<RawConfig> get(const std::string &path,
                                   bool create = false);
    std::shared_ptr<const RawConfig> get(const std::string &path) const;
    bool remove(const std::string &path);
    void removeAll();
    void setValue(std::string value);
    void setComment(std::string comment);
    void setLineNumber(unsigned int lineNumber);
    const std::string &name() const;
    const std::string &comment() const;
    const std::string &value() const;
    unsigned int lineNumber() const;
    bool hasSubItems() const;
    size_t subItemsSize() const;
    std::vector<std::string> subItems() const;
    void setValueByPath(const std::string &path, std::string value) {
        (*this)[path] = std::move(value);
    }

    const std::string *valueByPath(const std::string &path) const {
        auto config = get(path);
        return config ? &config->value() : nullptr;
    }

    RawConfig &operator[](const std::string &path) { return *get(path, true); }
    RawConfig &operator=(std::string value) {
        setValue(std::move(value));
        return *this;
    }

    bool operator==(const RawConfig &other) const {
        if (this == &other) {
            return true;
        }
        if (value() != other.value()) {
            return false;
        }
        if (subItemsSize() != other.subItemsSize()) {
            return false;
        }
        return visitSubItems(
            [&other](const RawConfig &subConfig, const std::string &path) {
                auto otherSubConfig = other.get(path);
                return (otherSubConfig && *otherSubConfig == subConfig);
            });
    }

    bool operator!=(const RawConfig &config) const {
        return !(*this == config);
    }

    RawConfig *parent() const;
    std::shared_ptr<RawConfig> detach();

    bool visitSubItems(
        std::function<bool(RawConfig &, const std::string &path)> callback,
        const std::string &path = "", bool recursive = false,
        const std::string &pathPrefix = "");
    bool visitSubItems(
        std::function<bool(const RawConfig &, const std::string &path)>
            callback,
        const std::string &path = "", bool recursive = false,
        const std::string &pathPrefix = "") const;
    void visitItemsOnPath(
        std::function<void(RawConfig &, const std::string &path)> callback,
        const std::string &path);
    void visitItemsOnPath(
        std::function<void(const RawConfig &, const std::string &path)>
            callback,
        const std::string &path) const;

private:
    friend class RawConfigPrivate;
    RawConfig(std::string name);
    std::shared_ptr<RawConfig> createSub(std::string name);
    FCITX_DECLARE_PRIVATE(RawConfig);
    std::unique_ptr<RawConfigPrivate> d_ptr;
};

FCITXUTILS_EXPORT
LogMessageBuilder &operator<<(LogMessageBuilder &log, const RawConfig &config);
} // namespace fcitx

#endif // _FCITX_CONFIG_RAWCONFIG_H_
