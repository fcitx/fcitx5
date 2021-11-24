/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _FCITX_INPUTCONTEXT_H_
#define _FCITX_INPUTCONTEXT_H_

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <fcitx-utils/capabilityflags.h>
#include <fcitx-utils/flags.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/rect.h>
#include <fcitx-utils/trackableobject.h>
#include <fcitx/event.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/surroundingtext.h>
#include <fcitx/text.h>
#include <fcitx/userinterface.h>
#include "fcitxcore_export.h"

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Input Context for Fcitx.

namespace fcitx {
typedef std::array<uint8_t, 16> ICUUID;

class InputContextManager;
class FocusGroup;
class InputContextPrivate;
class InputContextProperty;
class InputPanel;
class StatusArea;
typedef std::function<bool(InputContext *ic)> InputContextVisitor;

/**
 * An input context represents a client of Fcitx. It can be a Window, or a text
 * field depending on the application.
 *
 */
class FCITXCORE_EXPORT InputContext : public TrackableObject<InputContext> {
    friend class InputContextManagerPrivate;
    friend class FocusGroup;
    friend class UserInterfaceManager;

public:
    InputContext(InputContextManager &manager, const std::string &program = {});
    virtual ~InputContext();
    InputContext(InputContext &&other) = delete;

    /// Returns the underlying implementation of Input Context.
    virtual const char *frontend() const = 0;

    /// Returns the uuid of this input context.
    const ICUUID &uuid() const;

    /// Returns the program name of input context. It can be empty depending on
    /// the application.
    const std::string &program() const;

    /// Returns the display server of the client. In form of type:string, E.g.
    /// client from X11 server of :0 will become x11::0.
    std::string display() const;

    /// Returns the cursor position of the client.
    const Rect &cursorRect() const;

    /// Return the client scale factor.
    double scaleFactor() const;

    // Following functions should only be called by Frontend.
    // Calling following most of folloing functions will generate a
    // corresponding InputContextEvent.

    /// Called When input context gains the input focus.
    void focusIn();

    /// Called when input context losts the input focus.
    void focusOut();

    /**
     * Set the focus group of this input context. group can be null if it does
     * not belong to any group.
     *
     * @see FocusGroup
     */
    void setFocusGroup(FocusGroup *group);

    /// Returns the current focus group of input context.
    FocusGroup *focusGroup() const;

    /// Called when input context state need to be reset.
    FCITXCORE_DEPRECATED void reset(ResetReason reason);

    /// Called when input context state need to be reset.
    void reset();

    /**
     * Update the capability flags of the input context.
     *
     * @see CapabilityFlag
     */
    void setCapabilityFlags(CapabilityFlags flags);

    /**
     * Returns the current capability flags
     *
     * @see CapabilityFlag
     */
    CapabilityFlags capabilityFlags() const;

    /// Override the preedit hint from client.
    void setEnablePreedit(bool enable);

    /// Check if preedit is manually disalbed.
    bool isPreeditEnabled() const;

    /// Update the current cursor rect of the input context.
    void setCursorRect(Rect rect);

    /// Update the client rect with scale factor.
    void setCursorRect(Rect rect, double scale);

    /// Send a key event to current input context.
    bool keyEvent(KeyEvent &event);

    /// Returns whether the input context holds the input focus.
    bool hasFocus() const;

    /// Invoke an action on the preedit
    void invokeAction(InvokeActionEvent &event);

    /**
     * Returns the mutable surrounding text of the input context.
     *
     * updateSurroundingText() need to be called after changes by frontend.
     *
     * @see InputContext::updateSurroundingText SurroundingText
     */
    SurroundingText &surroundingText();

    /// Returns the immutable surrounding text of the input context.
    const SurroundingText &surroundingText() const;

    /// Notifies the surrounding text modification from the client.
    void updateSurroundingText();

    // Following functions are invoked by input method or misc modules to send
    // event to the corresponding client.
    /// Commit a string to the client.
    void commitString(const std::string &text);

    /// Ask client to delete a range of surrounding text.
    void deleteSurroundingText(int offset, unsigned int size);

    /// Send a key event to client.
    void forwardKey(const Key &rawKey, bool isRelease = false, int time = 0);

    /**
     * Notifies client about changes in clientPreedit
     *
     * @see InputPanel::clientPreedit
     */
    void updatePreedit();

    /**
     * Notifies UI about changes in user interface.
     *
     * @param component The components of UI that need to be updated.
     * @param immediate immediately flush the update to UI.
     *
     * @see UserInterfaceComponent InputPanel StatusArea
     */
    void updateUserInterface(UserInterfaceComponent component,
                             bool immediate = false);

    /**
     * Prevent event deliver to input context, and re-send the event later.
     *
     * This should be only used by frontend to make sync and async event handled
     * in the same order.
     *
     * @param block block state of input context.
     *
     * @see InputContextEventBlocker
     */
    void setBlockEventToClient(bool block);
    bool hasPendingEvents() const;

    /// Returns the input context property by name.
    InputContextProperty *property(const std::string &name);

    /// Returns the input context property by factory.
    InputContextProperty *property(const InputContextPropertyFactory *factory);

    /// Returns the associated input panel.
    InputPanel &inputPanel();

    /// Returns the associated StatusArea.
    StatusArea &statusArea();

    /**
     * Helper function to return the input context property in specific type.
     *
     * @param T type of the input context property.
     * @param name name of the input context property.
     * @return T*
     */
    template <typename T>
    T *propertyAs(const std::string &name) {
        return static_cast<T *>(property(name));
    }

    /**
     * Helper function to return the input context property in specific type by
     * given factory.
     *
     * @param T type of the input context property factory.
     * @param name name of the input context property.
     * @return T*
     */
    template <typename T>
    typename T::PropertyType *propertyFor(const T *factory) {
        return static_cast<typename T::PropertyType *>(property(factory));
    }

    /**
     * Notifes the change of a given input context property
     *
     * @param name name of the input context property.
     *
     * @see InputContextProperty::copyTo
     */
    void updateProperty(const std::string &name);

    /**
     * Notifes the change of a given input context property by its factory.
     *
     * @param name name of the input context property.
     *
     * @see InputContextProperty::copyTo
     */
    void updateProperty(const InputContextPropertyFactory *factory);

protected:
    /**
     * Send the committed string to client
     *
     * @param text string
     *
     * @see commitString
     */
    virtual void commitStringImpl(const std::string &text) = 0;

    /**
     * Send the delete Surrounding Text request to client.
     *
     * @param offset offset of deletion start, in UCS4 char.
     * @param size length of the deletion in UCS4 char.
     */
    virtual void deleteSurroundingTextImpl(int offset, unsigned int size) = 0;

    /**
     * Send the forwarded key to client
     *
     * @see forwardKey
     */
    virtual void forwardKeyImpl(const ForwardKeyEvent &key) = 0;

    /**
     * Send the preedit update to the client.
     *
     * @see updatePreedit
     */
    virtual void updatePreeditImpl() = 0;

    /**
     * Send the UI update to client.
     *
     * @see CapabilityFlag::ClientSideUI
     */
    virtual void updateClientSideUIImpl();

    /// Notifies the destruction of the input context. Need to be called in the
    /// destructor.
    void destroy();

    /// Notifies the creation of input context. Need to be called at the end of
    /// the constructor.
    void created();

private:
    void setHasFocus(bool hasFocus);

    std::unique_ptr<InputContextPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputContext);
};

/**
 * A helper class for frontend addon. It use RAII to call
 * setBlockEventToClient(true) in constructor and setBlockEventToClient(false)
 * in destructor.
 *
 */
class FCITXCORE_EXPORT InputContextEventBlocker {
public:
    InputContextEventBlocker(InputContext *inputContext);
    ~InputContextEventBlocker();

private:
    TrackableObjectReference<InputContext> inputContext_;
};

} // namespace fcitx

#endif // _FCITX_INPUTCONTEXT_H_
