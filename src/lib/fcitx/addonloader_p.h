/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_ADDONLOADER_P_H_
#define _FCITX_ADDONLOADER_P_H_

#include <string>
#include <fcitx/addonloader.h>

namespace fcitx {

class StaticLibraryLoader : public AddonLoader {
public:
    StaticLibraryLoader(StaticAddonRegistry *registry_);

    AddonInstance *load(const AddonInfo &info, AddonManager *manager) override;

    std::string type() const override { return "StaticLibrary"; }

    StaticAddonRegistry *registry;
};
} // namespace fcitx

#endif // _FCITX_ADDONLOADER_P_H_
