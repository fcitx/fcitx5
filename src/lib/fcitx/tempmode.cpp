/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "tempmode.h"
#include <memory>
#include "fcitx-utils/macros.h"
#include "event.h"
#include "inputcontext.h"
#include "inputcontextmanager.h"
#include "tempmode_p.h"

namespace fcitx {

TempMode::TempMode() : d_ptr(std::make_unique<TempModePrivate>(this)) {}
TempMode::~TempMode() { unregister(); }

bool TempMode::isRegistered() const {
    FCITX_D();
    return d->handle_ != nullptr;
}

bool TempMode::invokeAction(InvokeActionEvent &event) {
    FCITX_UNUSED(event);
    return true;
}

void TempMode::unregister() {
    FCITX_D();
    d->stateFactory_.unregister();
    d->handle_.reset();
}

InputContextProperty *
TempMode::genericProperty(InputContext *inputContext) const {
    FCITX_D();
    return inputContext->propertyFor(&d->stateFactory_);
}

} // namespace fcitx
