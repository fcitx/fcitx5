/*
 * SPDX-FileCopyrightText: 2016-2016 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fcitx-utils/library.h"
#include "fcitx-utils/log.h"
#include "addonloader_p.h"
#include "config.h"

namespace fcitx {

AddonLoader::~AddonLoader() {}

SharedLibraryLoader::~SharedLibraryLoader() {}

AddonInstance *SharedLibraryLoader::load(const AddonInfo &info,
                                         AddonManager *manager) {
    auto iter = registry_.find(info.uniqueName());
    if (iter == registry_.end()) {
        auto libs = standardPath_.locateAll(
            StandardPath::Type::Addon, info.library() + FCITX_LIBRARY_SUFFIX);
        for (const auto &libraryPath : libs) {
            Library lib(libraryPath);
            if (!lib.load()) {
                FCITX_ERROR()
                    << "Failed to load library for addon " << info.uniqueName()
                    << " on " << libraryPath << ". Error: " << lib.error();
                continue;
            }
            try {
                registry_.emplace(
                    info.uniqueName(),
                    std::make_unique<SharedLibraryFactory>(std::move(lib)));
            } catch (const std::exception &e) {
            }
            break;
        }
        iter = registry_.find(info.uniqueName());
    }

    if (iter == registry_.end()) {
        return nullptr;
    }

    try {
        return iter->second->factory()->create(manager);
    } catch (const std::exception &e) {
        FCITX_ERROR() << "Failed to create addon: " << info.uniqueName() << " "
                      << e.what();
    } catch (...) {
        FCITX_ERROR() << "Failed to create addon: " << info.uniqueName();
    }
    return nullptr;
}

StaticLibraryLoader::StaticLibraryLoader(StaticAddonRegistry *registry_)
    : AddonLoader(), registry(registry_) {}

AddonInstance *StaticLibraryLoader::load(const AddonInfo &info,
                                         AddonManager *manager) {
    auto iter = registry->find(info.uniqueName());
    if (iter == registry->end()) {
        return nullptr;
    }
    try {
        return iter->second->create(manager);
    } catch (...) {
        FCITX_ERROR() << "Failed to create addon: " << info.uniqueName();
    }
    return nullptr;
}
} // namespace fcitx
