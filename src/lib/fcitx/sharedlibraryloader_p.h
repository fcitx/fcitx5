/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_SHAREDLIBRARYLOADER_P_H_
#define _FCITX_SHAREDLIBRARYLOADER_P_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <fcitx/addonloader.h>

namespace fcitx {

class SharedLibraryFactory;

class SharedLibraryLoader : public AddonLoader {
public:
    SharedLibraryLoader();
    ~SharedLibraryLoader();
    AddonInstance *load(const AddonInfo &info, AddonManager *manager) override;

    std::string type() const override { return "SharedLibrary"; }

private:
    std::unordered_map<std::string, std::unique_ptr<SharedLibraryFactory>>
        registry_;
};

} // namespace fcitx

#endif // _FCITX_SHAREDLIBRARYLOADER_P_H_
