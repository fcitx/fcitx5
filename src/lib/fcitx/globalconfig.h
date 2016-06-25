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
#ifndef _FCITX_GLOBALCONFIG_H_
#define _FCITX_GLOBALCONFIG_H_

#include "fcitxcore_export.h"
#include <fcitx-utils/key.h>
#include <fcitx-utils/macros.h>
#include <memory>
#include <vector>

namespace fcitx {

class GlobalConfigPrivate;

class FCITXCORE_EXPORT GlobalConfig {
public:
    GlobalConfig();
    virtual ~GlobalConfig();
    const std::vector<Key> &triggerKeys() const;
    bool activeByDefault() const;

private:
    std::unique_ptr<GlobalConfigPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(GlobalConfig);
};
}

#endif // _FCITX_GLOBALCONFIG_H_
