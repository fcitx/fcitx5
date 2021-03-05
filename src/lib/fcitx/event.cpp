/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "event.h"

namespace fcitx {
Event::~Event() {}

KeyEventBase::KeyEventBase(EventType type, InputContext *context, Key rawKey,
                           bool isRelease, int time)
    : InputContextEvent(context, type), key_(rawKey.normalize()),
      origKey_(rawKey), rawKey_(rawKey), isRelease_(isRelease), time_(time) {}
} // namespace fcitx
