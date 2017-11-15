/*
 * Copyright (C) 2016~2016 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
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
#include <fcitx-config/rawconfig.h>
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
    const KeyList &triggerKeys() const;
    const KeyList &activateKeys() const;
    const KeyList &deactivateKeys() const;
    const KeyList &enumerateForwardKeys() const;
    const KeyList &enumerateBackwardKeys() const;
    const KeyList &enumerateGroupForwardKeys() const;
    const KeyList &enumerateGroupBackwardKeys() const;
    bool activeByDefault() const;
    bool showInputMethodInformation() const;

    const KeyList &defaultPrevPage() const;
    const KeyList &defaultNextPage() const;
    int defaultPageSize() const;

    const std::vector<std::string> &enabledAddons() const;
    const std::vector<std::string> &disabledAddons() const;

    void setEnabledAddons(const std::vector<std::string> &addons);
    void setDisabledAddons(const std::vector<std::string> &addons);

    void load(const RawConfig &rawConfig);
    void save(RawConfig &rawConfig) const;
    bool safeSave(const std::string &path = "config") const;

private:
    std::unique_ptr<GlobalConfigPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(GlobalConfig);
};
}

#endif // _FCITX_GLOBALCONFIG_H_
