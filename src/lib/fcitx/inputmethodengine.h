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
#ifndef _FCITX_INPUTMETHODENGINE_H_
#define _FCITX_INPUTMETHODENGINE_H_

#include "addoninstance.h"
#include "event.h"
#include "inputmethodentry.h"
#include "fcitxcore_export.h"

namespace fcitx {

class FCITXCORE_EXPORT InputMethodEngine : public AddonInstance {
public:
    virtual ~InputMethodEngine() {}

    virtual std::vector<InputMethodEntry> listInputMethods() = 0;
    virtual void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) = 0;
    virtual void save(const InputMethodEntry &) {}
    virtual void focusIn(const InputMethodEntry &) {}
    virtual void focusOut(const InputMethodEntry &) {}
    virtual void reset(const InputMethodEntry &) {}
    virtual void filterKey(const InputMethodEntry &, KeyEvent &) {}
    virtual void updateSurroundingText(const InputMethodEntry &) {}
    virtual std::string subMode(const InputMethodEntry &) { return {}; }
    virtual std::string overrideIcon(const InputMethodEntry &) { return {}; }
};
}

#endif // _FCITX_INPUTMETHODENGINE_H_
