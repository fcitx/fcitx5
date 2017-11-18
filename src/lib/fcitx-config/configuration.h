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
#ifndef _FCITX_CONFIG_CONFIGURATION_H_
#define _FCITX_CONFIG_CONFIGURATION_H_

#include "fcitxconfig_export.h"
#include <fcitx-config/option.h>
#include <fcitx-utils/macros.h>

#include <memory>

#define FCITX_CONFIGURATION(NAME, ...)                                         \
    class NAME;                                                                \
    FCITX_SPECIALIZE_TYPENAME(NAME, #NAME)                                     \
    FCITX_CONFIGURATION_CLASS(NAME, __VA_ARGS__)

#define FCITX_CONFIGURATION_CLASS(NAME, ...)                                   \
    class NAME : public ::fcitx::Configuration {                               \
    public:                                                                    \
        NAME() {}                                                              \
        NAME(const NAME &other) : NAME() { copyHelper(other); }                \
        NAME &operator=(const NAME &other) {                                   \
            copyHelper(other);                                                 \
            return *this;                                                      \
        }                                                                      \
        bool operator==(const NAME &other) const {                             \
            return compareHelper(other);                                       \
        }                                                                      \
        virtual const char *typeName() const override { return #NAME; }        \
                                                                               \
    public:                                                                    \
        __VA_ARGS__                                                            \
    };

#define FCITX_OPTION(name, type, path, description, default, ...)              \
    ::fcitx::Option<type> name {                                               \
        this, std::string(path), std::string(description), (default),          \
            __VA_ARGS__                                                        \
    }
namespace fcitx {

class ConfigurationPrivate;

class FCITXCONFIG_EXPORT Configuration {
    friend class OptionBase;

public:
    Configuration();
    virtual ~Configuration();

    /// Load configuration from RawConfig. If partial is true, non-exist option
    /// will be reset to default value, otherwise it will be untouched.
    void load(const RawConfig &config, bool partial = false);
    void save(RawConfig &config) const;
    void dumpDescription(RawConfig &config) const;
    virtual const char *typeName() const = 0;

protected:
    bool compareHelper(const Configuration &other) const;
    void copyHelper(const Configuration &other);

private:
    void addOption(fcitx::OptionBase *option);

    FCITX_DECLARE_PRIVATE(Configuration);
    std::unique_ptr<ConfigurationPrivate> d_ptr;
};
}

#endif // _FCITX_CONFIG_CONFIGURATION_H_
