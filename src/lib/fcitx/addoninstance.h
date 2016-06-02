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
#ifndef _FCITX_ADDONINSTANCE_H_
#define _FCITX_ADDONINSTANCE_H_

#include <functional>
#include "fcitxcore_export.h"
#include "fcitx-utils/library.h"

namespace fcitx {

class AddonInstance {
public:
    template <typename Signature>
    std::function<Signature> callback(const std::string &name) {
        return Library::toFunction<Signature>(rawCallback(name));
    }

    virtual void *rawCallback(const std::string &) { return nullptr; }
};
}

#define FCITX_PLUGIN_FACTORY(ClassName)                                        \
    extern "C" {                                                               \
    FCITXCORE_EXPORT                                                           \
    ::fcitx::AddonFactory *fcitx_addon_factory_instance() {                    \
        static ClassName factory;                                              \
        return &factory;                                                       \
    }                                                                          \
    }

#endif // _FCITX_ADDONINSTANCE_H_
