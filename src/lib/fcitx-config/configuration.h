/*
 * SPDX-FileCopyrightText: 2015-2015 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_CONFIG_CONFIGURATION_H_
#define _FCITX_CONFIG_CONFIGURATION_H_

#include <fcitx-config/option.h>
#include <fcitx-utils/macros.h>
#include "fcitxconfig_export.h"

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
        const char *typeName() const override { return #NAME; }                \
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

    /**
     * Set default value to current value.
     *
     * Sometimes, we need to customize the default value for the same type. This
     * function will set the default value to current value.
     */
    void syncDefaultValueToCurrent();

protected:
    bool compareHelper(const Configuration &other) const;
    void copyHelper(const Configuration &other);

private:
    void addOption(fcitx::OptionBase *option);

    FCITX_DECLARE_PRIVATE(Configuration);
    std::unique_ptr<ConfigurationPrivate> d_ptr;
};
} // namespace fcitx

#endif // _FCITX_CONFIG_CONFIGURATION_H_
