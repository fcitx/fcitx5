/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "addonloader.h"
#include <exception>
#include <string>
#include "fcitx-utils/log.h"
#include "addonfactory.h"
#include "addoninfo.h"
#include "addonloader_p.h"

namespace fcitx {

AddonLoader::~AddonLoader() {}

StaticLibraryLoader::StaticLibraryLoader(StaticAddonRegistry *registry_)
    : registry(registry_) {}

AddonInstance *StaticLibraryLoader::load(const AddonInfo &info,
                                         AddonManager *manager) {
    auto iter = registry->find(info.uniqueName());
    if (iter == registry->end()) {
        return nullptr;
    }
    try {
        return iter->second->create(manager);
    } catch (const std::exception &e) {
        FCITX_ERROR() << "Failed to create addon: " << info.uniqueName() << " "
                      << e.what();
    } catch (...) {
        FCITX_ERROR() << "Failed to create addon: " << info.uniqueName();
    }
    return nullptr;
}
} // namespace fcitx
