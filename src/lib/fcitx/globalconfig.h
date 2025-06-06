/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_GLOBALCONFIG_H_
#define _FCITX_GLOBALCONFIG_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <fcitx-config/configuration.h>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/macros.h>
#include <fcitx/fcitxcore_export.h>
#include <fcitx/inputcontextmanager.h>

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

    /**
     * Reset active state to the value of activeByDefault on Focus In.
     *
     * @see activeByDefault
     * @return the reset policy, no for disable, all for always reset, program
     * for only reset on program changes.
     * @since 5.1.8
     */
    PropertyPropagatePolicy resetStateWhenFocusIn() const;
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

    /**
     * Number of minutes that fcitx will automatically save user data.
     *
     * @return the period of auto save
     * @since 5.1.2
     */
    int autoSavePeriod() const;

    /**
     * Number of milliseconds that modifier only key can be triggered with key
     * release.
     *
     * @return timeout
     * @since 5.1.12
     */
    int modifierOnlyKeyTimeout() const;

    /**
     * Helper function to check whether the modifier only key should be
     * triggered.
     *
     * The user may need to record the time when the corresponding modifier only
     * key is pressed. The input time should use CLOCK_MONOTONIC.
     *
     * If timeout < 0, always return true.
     * Otherwise, check if it should be triggered based on current time.
     *
     * @return should trigger modifier only key
     */
    bool checkModifierOnlyKeyTimeout(uint64_t lastPressedTime) const;

    const std::vector<std::string> &enabledAddons() const;
    const std::vector<std::string> &disabledAddons() const;

    void setEnabledAddons(const std::vector<std::string> &addons);
    void setDisabledAddons(const std::vector<std::string> &addons);

    bool preloadInputMethod() const;

    void load(const RawConfig &rawConfig, bool partial = false);
    void save(RawConfig &rawConfig) const;
    bool safeSave(const std::string &path = "config") const;
    const Configuration &config() const;
    Configuration &config();

private:
    std::unique_ptr<GlobalConfigPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(GlobalConfig);
};
} // namespace fcitx

#endif // _FCITX_GLOBALCONFIG_H_
