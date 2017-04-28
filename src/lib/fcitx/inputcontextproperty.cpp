/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#include "inputcontextmanager.h"
#include "inputcontextproperty_p.h"

namespace fcitx {

InputContextPropertyFactory::InputContextPropertyFactory()
    : d_ptr(std::make_unique<InputContextPropertyFactoryPrivate>(this)) {}

InputContextPropertyFactory::~InputContextPropertyFactory() {
    FCITX_D();
    if (d->manager_) {
        d->manager_->unregisterProperty(d->name_);
    }
}
}
