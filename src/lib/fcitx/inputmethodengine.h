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

    /**
     * List the input methods provided by this engine.
     *
     * If OnDemand=True, the input method will be provided by configuration file
     * instead. Additional input method may be provided by configuration file.
     */
    virtual std::vector<InputMethodEntry> listInputMethods() { return {}; }

    /**
     * Main function where the input method handles a key event.
     *
     * @param entry input method entry
     * @param event key event
     */
    virtual void keyEvent(const InputMethodEntry &entry,
                          KeyEvent &keyEvent) = 0;
    /**
     * Called when the input context is switched to this input method.
     *
     * @param entry input method entry
     * @param event event
     */
    virtual void activate(const InputMethodEntry &entry,
                          InputContextEvent &event) {
        FCITX_UNUSED(entry);
        FCITX_UNUSED(event);
    }

    /**
     * Called when input context switch its input method.
     *
     * By default it will call reset.
     *
     * @param entry input method entry
     * @param event event
     */
    virtual void deactivate(const InputMethodEntry &entry,
                            InputContextEvent &event) {
        reset(entry, event);
    }
    /**
     * Being called when the input context need to reset it state.
     *
     * reset will only be called if ic is focused
     *
     * @param entry input method entry
     * @param event event
     */
    virtual void reset(const InputMethodEntry &entry,
                       InputContextEvent &event) {
        FCITX_UNUSED(entry);
        FCITX_UNUSED(event);
    }

    /**
     * If a key event is not handled by all other handler, it will be passed to
     * this function.
     *
     * This is useful for input method to block all the keys that it doesn't
     * want to handle.
     *
     * @param entry input method entry
     * @param event key event
     */
    virtual void filterKey(const InputMethodEntry &entry, KeyEvent &event) {
        FCITX_UNUSED(entry);
        FCITX_UNUSED(event);
    }

    FCITXCORE_DEPRECATED virtual void
    updateSurroundingText(const InputMethodEntry &) {}

    /**
     * Return a localized name for the sub mode of input method.
     *
     * @param entry input method entry
     * @param inputContext input context
     * @return name of the sub mode.
     */
    virtual std::string subMode(const InputMethodEntry &entry,
                                InputContext &inputContext) {
        FCITX_UNUSED(entry);
        FCITX_UNUSED(inputContext);
        return {};
    }

    /**
     * Return an alternative icon for entry.
     *
     * @param  entry input method entry
     * @return icon name
     *
     * @see InputMethodEngine::subModeIcon
     */
    virtual std::string overrideIcon(const InputMethodEntry &) { return {}; }

    /**
     * Return the configuration for this input method entry.
     *
     * The entry need to have Configurable=True
     * By default it will return the addon's config.
     *
     * @param entry input method entry
     * @return pointer to the configuration
     */
    virtual const Configuration *
    getConfigForInputMethod(const InputMethodEntry &entry) const {
        FCITX_UNUSED(entry);
        return getConfig();
    }

    /**
     * Update the configuration for this input method entry.
     *
     * The entry need to have Configurable=True
     * By default it will set the addon's config.
     *
     * @param entry input method entry
     */
    virtual void setConfigForInputMethod(const InputMethodEntry &entry,
                                         const RawConfig &config) {
        FCITX_UNUSED(entry);
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
     * @param  entry input method entry
     * @param  ic input context
     * @return std::string
     */
    std::string subModeLabel(const InputMethodEntry &entry, InputContext &ic);

    /**
     * Process InvokeActionEvent.
     *
     * @param entry input method entry
     * @param event Invoke action event
     * @since 5.0.11
     */
    void invokeAction(const InputMethodEntry &entry, InvokeActionEvent &event);
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

class FCITXCORE_EXPORT InputMethodEngineV3 : public InputMethodEngineV2 {
public:
    virtual void invokeActionImpl(const InputMethodEntry &entry,
                                  InvokeActionEvent &event);
};

} // namespace fcitx

#endif // _FCITX_INPUTMETHODENGINE_H_
