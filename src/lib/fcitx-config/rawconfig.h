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
#ifndef _FCITX_CONFIG_RAWCONFIG_H_
#define _FCITX_CONFIG_RAWCONFIG_H_

#include "fcitxconfig_export.h"
#include <algorithm>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace fcitx {

class RawConfig;

typedef std::shared_ptr<RawConfig> RawConfigPtr;

class RawConfigPrivate;
class FCITXCONFIG_EXPORT RawConfig {
public:
    explicit RawConfig(std::string name = "", std::string value = "");
    FCITX_DECLARE_VIRTUAL_DTOR_COPY_AND_MOVE(RawConfig)

    std::shared_ptr<RawConfig> get(const std::string &path,
                                   bool create = false);
    std::shared_ptr<const RawConfig> get(const std::string &path) const;
    bool remove(const std::string &path);
    void removeAll();
    void setValue(std::string value);
    void setComment(std::string value);
    void setLineNumber(unsigned int lineNumber);
    const std::string &name() const;
    const std::string &comment() const;
    const std::string &value() const;
    unsigned int lineNumber() const;
    bool hasSubItems() const;
    size_t subItemsSize() const;
    std::vector<std::string> subItems() const;
    void setValueByPath(const std::string &path, std::string value) {
        (*this)[path] = value;
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
                if (!otherSubConfig || *otherSubConfig != subConfig) {
                    return false;
                }
                return true;
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
    FCITX_DECLARE_PRIVATE(RawConfig);
    std::unique_ptr<RawConfigPrivate> d_ptr;
};

FCITXUTILS_EXPORT
LogMessageBuilder &operator<<(LogMessageBuilder &log, const RawConfig &config);
}

#endif // _FCITX_CONFIG_RAWCONFIG_H_
