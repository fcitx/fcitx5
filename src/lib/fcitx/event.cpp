/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "event.h"
#include <cstdint>
#include <memory>
#include <string>
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx-utils/macros.h"

namespace fcitx {

class VirtualKeyboardEventPrivate {
public:
    VirtualKeyboardEventPrivate(bool isRelease, int time)
        : isRelease_(isRelease), time_(time) {}

    Key key_;
    bool isRelease_ = false;
    int time_ = 0;
    uint64_t userAction_ = 0;
    std::string text_;
    bool isLongPress_ = false;
    float x_ = 0.0F, y_ = 0.0F;
};

Event::~Event() {}

KeyEventBase::KeyEventBase(EventType type, InputContext *context, Key rawKey,
                           bool isRelease, int time)
    : InputContextEvent(context, type), key_(rawKey.normalize()),
      origKey_(rawKey), rawKey_(rawKey), isRelease_(isRelease), time_(time) {}

VirtualKeyboardEvent::VirtualKeyboardEvent(InputContext *context,
                                           bool isRelease, int time)
    : InputContextEvent(context, EventType::InputContextVirtualKeyboardEvent),
      d_ptr(std::make_unique<VirtualKeyboardEventPrivate>(isRelease, time)) {}

FCITX_DEFINE_DEFAULT_DTOR(VirtualKeyboardEvent);

FCITX_DEFINE_PROPERTY_PRIVATE(VirtualKeyboardEvent, Key, key, setKey);
FCITX_DEFINE_PROPERTY_PRIVATE(VirtualKeyboardEvent, uint64_t, userAction,
                              setUserAction);
FCITX_DEFINE_PROPERTY_PRIVATE(VirtualKeyboardEvent, std::string, text, setText);
FCITX_DEFINE_PROPERTY_PRIVATE(VirtualKeyboardEvent, bool, isLongPress,
                              setLongPress);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(VirtualKeyboardEvent, float, x);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(VirtualKeyboardEvent, float, y);
FCITX_DEFINE_READ_ONLY_PROPERTY_PRIVATE(VirtualKeyboardEvent, int, time);

void VirtualKeyboardEvent::setPosition(float x, float y) {
    FCITX_D();
    d->x_ = x;
    d->y_ = y;
}

std::unique_ptr<KeyEvent> fcitx::VirtualKeyboardEvent::toKeyEvent() const {
    FCITX_D();
    if (!d->key_.isValid()) {
        return nullptr;
    }

    Key key{d->key_.sym(), d->key_.states() | KeyState::Virtual,
            d->key_.code()};
    return std::make_unique<KeyEvent>(inputContext(), key, d->isRelease_,
                                      time());
}

} // namespace fcitx
