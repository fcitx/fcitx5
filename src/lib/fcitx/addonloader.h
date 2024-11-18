/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_ADDONLOADER_H_
#define _FCITX_ADDONLOADER_H_

#include <string>
#include <unordered_map>
#include <fcitx/addoninfo.h>
#include <fcitx/addoninstance.h>
#include "fcitxcore_export.h"

namespace fcitx {

class AddonFactory;
class AddonManager;

using StaticAddonRegistry = std::unordered_map<std::string, AddonFactory *>;

class FCITXCORE_EXPORT AddonLoader {
public:
    virtual ~AddonLoader();
    virtual std::string type() const = 0;
    virtual AddonInstance *load(const AddonInfo &info,
                                AddonManager *manager) = 0;
};
} // namespace fcitx

#endif // _FCITX_ADDONLOADER_H_
