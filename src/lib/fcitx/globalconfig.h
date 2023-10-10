/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_GLOBALCONFIG_H_
#define _FCITX_GLOBALCONFIG_H_

#include <memory>
#include <vector>
#include <fcitx-config/configuration.h>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/macros.h>
#include <fcitx/inputcontextmanager.h>
#include "fcitxcore_export.h"

namespace fcitx {

class GlobalConfigPrivate;

class FCITXCORE_EXPORT GlobalConfig {
public:
    GlobalConfig();
    virtual ~GlobalConfig();
    const KeyList &triggerKeys() const;
    bool enumerateWithTriggerKeys() const;
    const KeyList &altTriggerKeys() const;
    const KeyList &activateKeys() const;
    const KeyList &deactivateKeys() const;
    const KeyList &enumerateForwardKeys() const;
    const KeyList &enumerateBackwardKeys() const;
    bool enumerateSkipFirst() const;
    const KeyList &enumerateGroupForwardKeys() const;
    const KeyList &enumerateGroupBackwardKeys() const;
    const KeyList &togglePreeditKeys() const;

    bool activeByDefault() const;
    bool showInputMethodInformation() const;
    bool showInputMethodInformationWhenFocusIn() const;
    bool compactInputMethodInformation() const;
    bool showFirstInputMethodInformation() const;
    PropertyPropagatePolicy shareInputState() const;
    bool preeditEnabledByDefault() const;

    const KeyList &defaultPrevPage() const;
    const KeyList &defaultNextPage() const;

    const KeyList &defaultPrevCandidate() const;
    const KeyList &defaultNextCandidate() const;
    int defaultPageSize() const;

    /**
     * Override the xkb option from display.
     *
     * This is only useful for custom xkb translation.
     * Right now there is no way to read system xkb option from wayland.
     * Instead, we can ask user for the fallback.
     *
     * @return override xkb option from system or not
     * @since 5.0.14
     */
    bool overrideXkbOption() const;

    /**
     * The enforce the xkb option for custom xkb state.
     *
     * This is only useful for custom xkb translation.
     * Right now there is no way to read system xkb option from wayland.
     * Instead, we can ask user for the fallback.
     *
     * @return override xkb option
     * @see GlobalConfig::overrideXkbOption
     * @since 5.0.14
     */
    const std::string &customXkbOption() const;

    /**
     * Allow use input method in password field.
     *
     * @return whether allow use input method in password field.
     * @since 5.1.2
     */
    bool allowInputMethodForPassword() const;

    /**
     * Show preedit when typing in password field.
     *
     * @return whether show preedit in password field.
     * @since 5.1.2
     */
    bool showPreeditForPassword() const;

    const std::vector<std::string> &enabledAddons() const;
    const std::vector<std::string> &disabledAddons() const;

    void setEnabledAddons(const std::vector<std::string> &addons);
    void setDisabledAddons(const std::vector<std::string> &addons);

    bool preloadInputMethod() const;

    void load(const RawConfig &rawConfig, bool partial = false);
    void save(RawConfig &rawConfig) const;
    bool safeSave(const std::string &path = "config") const;
    const Configuration &config() const;

private:
    std::unique_ptr<GlobalConfigPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(GlobalConfig);
};
} // namespace fcitx

#endif // _FCITX_GLOBALCONFIG_H_
