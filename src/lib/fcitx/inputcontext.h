/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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
#include <fcitx-utils/flags.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/rect.h>
#include <fcitx/event.h>
#include <fcitx/inputpanel.h>
#include <fcitx/surroundingtext.h>
#include <fcitx/text.h>
#include <memory>
#include <string>
#include <uuid/uuid.h>

namespace fcitx {

// Use uint64_t for more space for future
enum class CapabilityFlag : uint64_t {
    None = 0,
    ClientSideUI = (1 << 0),
    Preedit = (1 << 1),
    ClientSideControlState = (1 << 2),
    Password = (1 << 3),
    FormattedPreedit = (1 << 4),
    ClientUnfocusCommit = (1 << 5),
    SurroundingText = (1 << 6),
    Email = (1 << 7),
    Digit = (1 << 8),
    Uppercase = (1 << 9),
    Lowercase = (1 << 10),
    NoAutoUpperCase = (1 << 11),
    Url = (1 << 12),
    Dialable = (1 << 13),
    Number = (1 << 14),
    NoOnScreenKeyboard = (1 << 15),
    SpellCheck = (1 << 16),
    NoSpellCheck = (1 << 17),
    WordCompletion = (1 << 18),
    UppercaseWords = (1 << 19),
    UppwercaseSentences = (1 << 20),
    Alpha = (1 << 21),
    Name = (1 << 22),
    RelativeRect = (1 << 23),
    Terminal = (1 << 24),
    Date = (1 << 25),
    Time = (1 << 26),
    Multiline = (1 << 27),
    Sensitive = (1 << 28),
    HiddenText = (1 << 29),
};

typedef Flags<CapabilityFlag> CapabilityFlags;
typedef std::array<uint8_t, sizeof(uuid_t)> ICUUID;

class InputContextManager;
class FocusGroup;
class InputContextPrivate;
class InputContextProperty;

class FCITXCORE_EXPORT InputContext {
    friend class InputContextManagerPrivate;
    friend class FocusGroup;

public:
    InputContext(InputContextManager &manager, const std::string &program = {});
    virtual ~InputContext();
    InputContext(InputContext &&other) = delete;

    const ICUUID &uuid() const;
    const std::string &program() const;
    std::string display() const;

    void focusIn();
    void focusOut();
    void setFocusGroup(FocusGroup *group);
    FocusGroup *focusGroup() const;
    void reset();
    void setCapabilityFlags(CapabilityFlags flags);
    CapabilityFlags capabilityFlags();
    void setCursorRect(Rect rect);
    bool keyEvent(KeyEvent &key);

    bool hasFocus() const;

    SurroundingText &surroundingText();
    const SurroundingText &surroundingText() const;
    void updateSurroundingText();

    void commitString(const std::string &text);
    void deleteSurroundingText(int offset, unsigned int size);
    void forwardKey(const Key &rawKey, bool isRelease = false, int keyCode = 0, int time = 0);
    void updatePreedit();

    InputContextProperty *property(const std::string &name);

    InputPanel &inputPanel();

    template <typename T>
    T *propertyAs(const std::string &name) {
        return static_cast<T *>(property(name));
    }

    void updateProperty(const std::string &name);

protected:
    virtual void commitStringImpl(const std::string &text) = 0;
    virtual void deleteSurroundingTextImpl(int offset, unsigned int size) = 0;
    virtual void forwardKeyImpl(const ForwardKeyEvent &key) = 0;
    virtual void updatePreeditImpl() = 0;

    void registerProperty(const std::string &name, InputContextProperty *property);
    void unregisterProperty(const std::string &name);

private:
    void setHasFocus(bool hasFocus);

    std::unique_ptr<InputContextPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(InputContext);
};
}

#endif // _FCITX_INPUTCONTEXT_H_
