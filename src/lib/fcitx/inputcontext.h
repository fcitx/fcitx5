/*
 * Copyright (C) 2016~2016 by CSSlayer
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

#ifndef _FCITX_INPUTCONTEXT_H_
#define _FCITX_INPUTCONTEXT_H_

#include "fcitxcore_export.h"
#include <array>
#include <cstdint>
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
#include <memory>
#include <string>
#include <uuid/uuid.h>

namespace fcitx {
typedef std::array<uint8_t, sizeof(uuid_t)> ICUUID;

class InputContextManager;
class FocusGroup;
class InputContextPrivate;
class InputContextProperty;
class InputPanel;
class StatusArea;

class FCITXCORE_EXPORT InputContext : public TrackableObject<InputContext> {
    friend class InputContextManagerPrivate;
    friend class FocusGroup;

public:
    InputContext(InputContextManager &manager, const std::string &program = {});
    virtual ~InputContext();
    InputContext(InputContext &&other) = delete;

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

    InputContextProperty *property(const std::string &name);
    InputContextProperty *property(InputContextPropertyFactory *factory);

    InputPanel &inputPanel();
    StatusArea &statusArea();

    template <typename T>
    T *propertyAs(const std::string &name) {
        return static_cast<T *>(property(name));
    }

    template <typename T>
    typename T::PropertyType *propertyFor(T *factory) {
        return static_cast<typename T::PropertyType *>(property(factory));
    }

    void updateProperty(const std::string &name);

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
}

#endif // _FCITX_INPUTCONTEXT_H_
