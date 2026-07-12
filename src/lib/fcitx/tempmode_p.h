/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _FCITX_TEMPMODE_P_H_
#define _FCITX_TEMPMODE_P_H_

#include <memory>
#include <string>
#include <utility>
#include "fcitx-utils/handlertable.h"
#include "fcitx-utils/macros.h"
#include "fcitx/inputcontextproperty.h"
#include "instance.h"
#include "tempmode.h"

namespace fcitx {

class InputContextManager;

class TempModePrivate : public QPtrHolder<TempMode> {
public:
    explicit TempModePrivate(TempMode *q)
        : QPtrHolder(q), stateFactory_([this](InputContext &inputContext) {
              FCITX_Q();
              return q->createProperty(inputContext);
          }) {}

    void registerCallback(std::unique_ptr<HandlerTableEntry<TempMode *>> handle,
                          Instance *instance) {
        FCITX_Q();
        handle_ = std::move(handle);
        instance->inputContextManager().registerProperty(std::string(q->name()),
                                                         &stateFactory_);
    }

    TempMode *q_;
    std::unique_ptr<HandlerTableEntry<TempMode *>> handle_;
    FactoryFor<InputContextProperty> stateFactory_;
};

} // namespace fcitx

#endif // _FCITX_TEMPMODE_P_H_
