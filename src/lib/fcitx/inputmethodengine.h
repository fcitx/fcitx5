/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INPUTMETHODENGINE_H_
#define _FCITX_INPUTMETHODENGINE_H_

#include <fcitx/addoninstance.h>
#include <fcitx/event.h>
#include <fcitx/inputmethodentry.h>
#include "fcitxcore_export.h"

namespace fcitx {

class FCITXCORE_EXPORT InputMethodEngine : public AddonInstance {
public:
    virtual ~InputMethodEngine() {}

    virtual std::vector<InputMethodEntry> listInputMethods() { return {}; }
    virtual void keyEvent(const InputMethodEntry &entry,
                          KeyEvent &keyEvent) = 0;
    // fcitx gurantee that activate and deactivate appear in pair for all input
    // context
    // activate means it will be used.
    virtual void activate(const InputMethodEntry &, InputContextEvent &) {}
    // deactivate means it will not be used for this context
    virtual void deactivate(const InputMethodEntry &entry,
                            InputContextEvent &event) {
        reset(entry, event);
    }
    // reset will only be called if ic is focused
    virtual void reset(const InputMethodEntry &, InputContextEvent &) {}
    virtual void filterKey(const InputMethodEntry &, KeyEvent &) {}
    FCITXCORE_DEPRECATED virtual void
    updateSurroundingText(const InputMethodEntry &) {}
    virtual std::string subMode(const InputMethodEntry &, InputContext &) {
        return {};
    }
    virtual std::string overrideIcon(const InputMethodEntry &) { return {}; }
    virtual const Configuration *
    getConfigForInputMethod(const InputMethodEntry &) const {
        return getConfig();
    }
    virtual void setConfigForInputMethod(const InputMethodEntry &,
                                         const RawConfig &config) {
        setConfig(config);
    }
    /**
     * Return the icon name for the sub mode.
     *
     * Prefer subclass this method from InputMethodEngineV2 over overrideIcon.
     *
     * @param  entry input method entry
     * @param  ic input context
     * @return std::string
     *
     * @see overrideIcon
     */
    std::string subModeIcon(const InputMethodEntry &entry, InputContext &ic);

    /**
     * Return the label for the sub mode.
     *
     * Prefer subclass this method from InputMethodEngineV2 over overrideIcon.
     *
     * @param  entry input method entry
     * @param  ic input context
     * @return std::string
     *
     * @see overrideIcon
     */
    std::string subModeLabel(const InputMethodEntry &entry, InputContext &ic);
};

class FCITXCORE_EXPORT InputMethodEngineV2 : public InputMethodEngine {
public:
    virtual std::string subModeIconImpl(const InputMethodEntry &,
                                        InputContext &) {
        return {};
    }
    virtual std::string subModeLabelImpl(const InputMethodEntry &,
                                         InputContext &) {
        return {};
    }
};

} // namespace fcitx

#endif // _FCITX_INPUTMETHODENGINE_H_
