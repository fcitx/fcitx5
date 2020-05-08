/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "inputcontextmanager.h"
#include "inputcontextproperty_p.h"

namespace fcitx {

InputContextPropertyFactory::InputContextPropertyFactory()
    : d_ptr(std::make_unique<InputContextPropertyFactoryPrivate>(this)) {}

InputContextPropertyFactory::~InputContextPropertyFactory() { unregister(); }

bool InputContextPropertyFactory::registered() const {
    FCITX_D();
    return d->manager_;
}

void InputContextPropertyFactory::unregister() {
    FCITX_D();
    if (d->manager_) {
        d->manager_->unregisterProperty(d->name_);
    }
}
} // namespace fcitx
