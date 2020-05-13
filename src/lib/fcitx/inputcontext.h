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

namespace fcitx {
typedef std::array<uint8_t, 16> ICUUID;

class InputContextManager;
class FocusGroup;
class InputContextPrivate;
class InputContextProperty;
class InputPanel;
class StatusArea;
typedef std::function<bool(InputContext *ic)> InputContextVisitor;

class FCITXCORE_EXPORT InputContext : public TrackableObject<InputContext> {
    friend class InputContextManagerPrivate;
    friend class FocusGroup;

public:
    InputContext(InputContextManager &manager, const std::string &program = {});
    virtual ~InputContext();
    InputContext(InputContext &&other) = delete;

    virtual const char *frontend() const = 0;

    const ICUUID &uuid() const;
    const std::string &program() const;
    std::string display() const;
    const Rect &cursorRect() const;

    void focusIn();
    void focusOut();
    void setFocusGroup(FocusGroup *group);
    FocusGroup *focusGroup() const;
    void reset(ResetReason reason);
    void setCapabilityFlags(CapabilityFlags flags);
    CapabilityFlags capabilityFlags() const;
    void setCursorRect(Rect rect);
    bool keyEvent(KeyEvent &key);

    bool hasFocus() const;

    SurroundingText &surroundingText();
    const SurroundingText &surroundingText() const;
    void updateSurroundingText();

    void commitString(const std::string &text);
    void deleteSurroundingText(int offset, unsigned int size);
    void forwardKey(const Key &rawKey, bool isRelease = false, int time = 0);
    void updatePreedit();
    void updateUserInterface(UserInterfaceComponent componet,
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

    InputContextProperty *property(const std::string &name);
    InputContextProperty *property(const InputContextPropertyFactory *factory);

    InputPanel &inputPanel();
    StatusArea &statusArea();

    template <typename T>
    T *propertyAs(const std::string &name) {
        return static_cast<T *>(property(name));
    }

    template <typename T>
    typename T::PropertyType *propertyFor(const T *factory) {
        return static_cast<typename T::PropertyType *>(property(factory));
    }

    void updateProperty(const std::string &name);
    void updateProperty(const InputContextPropertyFactory *factory);

protected:
    virtual void commitStringImpl(const std::string &text) = 0;
    virtual void deleteSurroundingTextImpl(int offset, unsigned int size) = 0;
    virtual void forwardKeyImpl(const ForwardKeyEvent &key) = 0;
    virtual void updatePreeditImpl() = 0;
    void destroy();
    void created();

private:
    void setHasFocus(bool hasFocus);

    std::unique_ptr<InputContextPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputContext);
};

class FCITXCORE_EXPORT InputContextEventBlocker {
public:
    InputContextEventBlocker(InputContext *inputContext);
    ~InputContextEventBlocker();

private:
    TrackableObjectReference<InputContext> inputContext_;
};

} // namespace fcitx

#endif // _FCITX_INPUTCONTEXT_H_
