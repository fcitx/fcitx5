/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _FCITX_TEMPMODE_H_
#define _FCITX_TEMPMODE_H_

#include <memory>
#include <string_view>
#include <fcitx-utils/key.h>
#include <fcitx-utils/macros.h>
#include <fcitx/event.h>
#include <fcitx/fcitxcore_export.h>
#include <fcitx/inputcontextproperty.h>

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Temporary input modes for Fcitx.

namespace fcitx {

class TempModePrivate;

template <typename PropertyBaseType>
class SimpleTempModeState : public InputContextProperty,
                            public PropertyBaseType {
public:
    bool isActive() const { return active_; }
    void setActive(bool active) { active_ = active; }

private:
    bool active_ = false;
};

/**
 * A temporary mode that can take over key handling for a short period.
 *
 * A temp mode is typically activated by a hotkey, handles subsequent key
 * events while active, and then returns control to the regular input method.
 */
class FCITXCORE_EXPORT TempMode {
public:
    friend class TempModeManager;
    friend class TempModePrivate;

    TempMode();
    virtual ~TempMode();

    /**
     * Return whether this temp mode is registered with a manager.
     */
    bool isRegistered() const;

    /**
     * Unregister this temp mode from its manager.
     */
    void unregister();

    /**
     * Return whether this temp mode is currently active.
     */
    virtual bool isActive(InputContext *inputContext) const = 0;

    /**
     * Handle a potential trigger key event.
     *
     * The default implementation checks triggerKeys() against the key event and
     * activates the mode on a matching key press.
     *
     * @param keyEvent key event
     * @return whether the key event activates the temp mode
     */
    virtual bool triggerTempMode(const KeyEvent &keyEvent) = 0;

    /**
     * Handle a key event while the temp mode is active.
     *
     * @param keyEvent key event
     * @return whether the key event should be accepted
     */
    virtual bool keyEvent(const KeyEvent &keyEvent) = 0;

    /**
     * Handle an action invoked while the temp mode is active.
     *
     * @param event action event
     * @return whether the action event should be accepted
     */
    virtual bool invokeAction(InvokeActionEvent &event);

    /**
     * Reset the temp mode state for an input context.
     *
     * @param inputContext input context
     */
    virtual void reset(InputContext *inputContext) = 0;

    /**
     * Return the input context property name
     */
    virtual std::string_view name() const = 0;

protected:
    /**
     * Create the property object stored for each input context.
     */
    virtual InputContextProperty *
    createProperty(InputContext &inputContext) = 0;

    /**
     * Return the registered temp mode state for an input context.
     */
    InputContextProperty *genericProperty(InputContext *inputContext) const;

private:
    std::unique_ptr<TempModePrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TempMode);
};

template <typename PropertyBaseType>
class SimpleTempMode : public TempMode {
public:
    using PropertyType = SimpleTempModeState<PropertyBaseType>;

    PropertyType *property(InputContext *inputContext) const {
        return static_cast<PropertyType *>(genericProperty(inputContext));
    }

protected:
    bool isActive(InputContext *inputContext) const override {
        if (auto *prop = property(inputContext)) {
            return prop->isActive();
        }
        return false;
    }

    bool triggerTempMode(const KeyEvent &keyEvent) override {
        if (keyEvent.isRelease()) {
            return false;
        }
        if (keyEvent.key().checkKeyList(triggerKeys())) {
            if (auto *prop = property(keyEvent.inputContext())) {
                prop->setActive(true);
            }
            return true;
        }
        return false;
    }

    void reset(InputContext *inputContext) override {
        if (auto *prop = property(inputContext)) {
            prop->setActive(false);
        }
    }

    InputContextProperty *createProperty(InputContext &inputContext) override {
        FCITX_UNUSED(inputContext);
        return new PropertyType;
    }

    /**
     * Return the hotkeys that can activate this temp mode.
     */
    virtual const KeyList &triggerKeys() const = 0;
};

} // namespace fcitx

/// \}

#endif // _FCITX_TEMPMODE_H_
