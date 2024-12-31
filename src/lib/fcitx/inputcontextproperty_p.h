/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_INPUTCONTEXTPROPERTY_P_H_
#define _FCITX_INPUTCONTEXTPROPERTY_P_H_

#include <string>
#include <fcitx/inputcontextproperty.h>
#include "fcitx-utils/macros.h"
#include "fcitx/inputcontextmanager.h"

namespace fcitx {
class InputContextManagerPrivate;
class InputContextPropertyFactoryPrivate
    : public QPtrHolder<InputContextPropertyFactory> {
public:
    InputContextPropertyFactoryPrivate(InputContextPropertyFactory *q)
        : QPtrHolder(q) {}
    InputContextManager *manager_ = nullptr;
    int slot_ = -1;
    std::string name_;
};
} // namespace fcitx

#endif // _FCITX_INPUTCONTEXTPROPERTY_P_H_
